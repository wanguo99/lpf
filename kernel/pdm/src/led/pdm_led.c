// SPDX-License-Identifier: GPL-2.0

#include "osal.h"
#include "hal.h"
#include "pconfig.h"
#include "pdm.h"
#include "pdm_internal.h"
#include "pdm_led_internal.h"

typedef struct {
	const pconfig_led_config_t *config;
	hal_pwm_handle_t pwm;
	osal_mutex_t lock;
	uint32_t brightness;
	bool enabled;
	bool lock_ready;
} pdm_led_context_t;

static pdm_led_context_t *g_led_contexts[PDM_LED_MAX_DEVICES];
static osal_mutex_t g_led_registry_lock;
static bool g_led_registry_ready;

static uint32_t pdm_led_clamp_brightness(const pdm_led_context_t *ctx,
					 uint32_t brightness)
{
	if (brightness > ctx->config->max_brightness)
		return ctx->config->max_brightness;

	return brightness;
}

static hal_gpio_level_t pdm_led_gpio_level(const pconfig_led_config_t *config,
					   bool enabled)
{
	bool active = enabled;

	if (config->hw.gpio.active_low)
		active = !active;

	return active ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
}

static int32_t pdm_led_apply_gpio(pdm_led_context_t *ctx)
{
	hal_gpio_level_t level;

	level = pdm_led_gpio_level(ctx->config, ctx->enabled);
	return hal_gpio_set_level(ctx->config->hw.gpio.gpio_num, level);
}

static int32_t pdm_led_apply_pwm(pdm_led_context_t *ctx)
{
	hal_pwm_state_t state;
	uint64_t duty;

	if (ctx->config->max_brightness == 0)
		return OSAL_ERR_INVALID_PARAM;

	duty = (uint64_t)ctx->config->hw.pwm.period_ns * ctx->brightness;
	duty /= ctx->config->max_brightness;

	state.period_ns = ctx->config->hw.pwm.period_ns;
	state.duty_ns = (uint32_t)duty;
	state.enabled = ctx->enabled && ctx->brightness > 0;
	state.polarity_inversed = ctx->config->hw.pwm.polarity_inversed;
	return hal_pwm_apply(ctx->pwm, &state);
}

static int32_t pdm_led_apply(pdm_led_context_t *ctx)
{
	switch (ctx->config->control) {
	case PCONFIG_LED_CONTROL_GPIO:
		return pdm_led_apply_gpio(ctx);
	case PCONFIG_LED_CONTROL_PWM:
		return pdm_led_apply_pwm(ctx);
	default:
		return OSAL_ERR_INVALID_PARAM;
	}
}

static int32_t pdm_led_registry_init(void)
{
	if (g_led_registry_ready)
		return OSAL_SUCCESS;

	if (osal_mutex_init(&g_led_registry_lock, NULL) != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	g_led_registry_ready = true;
	return OSAL_SUCCESS;
}

static int32_t pdm_led_init_gpio(pdm_led_context_t *ctx)
{
	hal_gpio_config_t gpio_config;

	gpio_config.direction = HAL_GPIO_DIR_OUTPUT;
	gpio_config.initial_level = pdm_led_gpio_level(ctx->config,
						       ctx->enabled);
	gpio_config.edge = HAL_GPIO_EDGE_NONE;
	gpio_config.callback = NULL;
	gpio_config.user_data = NULL;

	return hal_gpio_init(ctx->config->hw.gpio.gpio_num, &gpio_config);
}

static int32_t pdm_led_init_pwm(pdm_led_context_t *ctx)
{
	hal_pwm_config_t pwm_config;

	if (!ctx->config->hw.pwm.consumer ||
	    ctx->config->hw.pwm.period_ns == 0)
		return OSAL_ERR_INVALID_PARAM;

	pwm_config.consumer = ctx->config->hw.pwm.consumer;
	pwm_config.period_ns = ctx->config->hw.pwm.period_ns;
	pwm_config.duty_ns = 0;
	pwm_config.enabled = false;
	pwm_config.polarity_inversed = ctx->config->hw.pwm.polarity_inversed;

	return hal_pwm_init(&pwm_config, &ctx->pwm);
}

static void pdm_led_deinit_hw(pdm_led_context_t *ctx)
{
	if (!ctx)
		return;

	switch (ctx->config->control) {
	case PCONFIG_LED_CONTROL_GPIO:
		hal_gpio_deinit(ctx->config->hw.gpio.gpio_num);
		break;
	case PCONFIG_LED_CONTROL_PWM:
		if (ctx->pwm)
			hal_pwm_deinit(ctx->pwm);
		ctx->pwm = NULL;
		break;
	default:
		break;
	}
}

static int32_t pdm_led_init_from_entry(uint32_t index,
				       const pconfig_led_entry_t *entry,
				       pdm_led_handle_t *handle)
{
	pdm_led_context_t *ctx;
	int32_t ret;

	if (!handle || !entry || !entry->enabled ||
	    index >= OSAL_ARRAY_SIZE(g_led_contexts))
		return OSAL_ERR_INVALID_PARAM;

	if (entry->config.max_brightness == 0)
		return OSAL_ERR_INVALID_PARAM;

	ret = pdm_led_registry_init();
	if (ret != OSAL_SUCCESS)
		return ret;

	osal_mutex_lock(&g_led_registry_lock);
	if (g_led_contexts[index]) {
		*handle = g_led_contexts[index];
		osal_mutex_unlock(&g_led_registry_lock);
		return OSAL_SUCCESS;
	}
	osal_mutex_unlock(&g_led_registry_lock);

	ctx = osal_zalloc(sizeof(*ctx));
	if (!ctx)
		return OSAL_ERR_NO_MEMORY;

	ctx->config = &entry->config;
	ctx->brightness = pdm_led_clamp_brightness(ctx,
						   entry->config.default_brightness);
	ctx->enabled = ctx->brightness > 0;

	ret = osal_mutex_init(&ctx->lock, NULL);
	if (ret != OSAL_SUCCESS) {
		osal_free(ctx);
		return ret;
	}
	ctx->lock_ready = true;

	switch (entry->config.control) {
	case PCONFIG_LED_CONTROL_GPIO:
		ret = pdm_led_init_gpio(ctx);
		break;
	case PCONFIG_LED_CONTROL_PWM:
		ret = pdm_led_init_pwm(ctx);
		break;
	default:
		ret = OSAL_ERR_INVALID_PARAM;
		break;
	}
	if (ret != OSAL_SUCCESS)
		goto out_free;

	ret = pdm_led_apply(ctx);
	if (ret != OSAL_SUCCESS)
		goto out_hw;

	osal_mutex_lock(&g_led_registry_lock);
	if (g_led_contexts[index]) {
		*handle = g_led_contexts[index];
		osal_mutex_unlock(&g_led_registry_lock);
		pdm_led_deinit_hw(ctx);
		goto out_free_success;
	}
	g_led_contexts[index] = ctx;
	osal_mutex_unlock(&g_led_registry_lock);

	*handle = ctx;
	return OSAL_SUCCESS;

out_hw:
	pdm_led_deinit_hw(ctx);
out_free:
	if (ctx->lock_ready)
		osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
	return ret;
out_free_success:
	if (ctx->lock_ready)
		osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
	return OSAL_SUCCESS;
}

int32_t pdm_led_probe(const lpf_device_t *device)
{
	const pconfig_led_entry_t *entry;
	pdm_led_handle_t handle = NULL;

	if (!device || device->config.type != LPF_DEVICE_TYPE_LED)
		return OSAL_ERR_INVALID_PARAM;

	entry = (const pconfig_led_entry_t *)device->config.entry;
	return pdm_led_init_from_entry(device->config.index, entry, &handle);
}

pdm_led_handle_t pdm_led_get(uint32_t index)
{
	pdm_led_handle_t handle = NULL;

	if (!g_led_registry_ready || index >= OSAL_ARRAY_SIZE(g_led_contexts))
		return NULL;

	osal_mutex_lock(&g_led_registry_lock);
	handle = g_led_contexts[index];
	osal_mutex_unlock(&g_led_registry_lock);

	return handle;
}

int32_t pdm_led_debug_get(uint32_t index, pdm_led_debug_info_t *info)
{
	pdm_led_context_t *ctx;

	if (!info || index >= OSAL_ARRAY_SIZE(g_led_contexts))
		return OSAL_ERR_INVALID_PARAM;

	osal_memset(info, 0, sizeof(*info));

	if (!g_led_registry_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_led_registry_lock);
	ctx = g_led_contexts[index];
	if (ctx) {
		osal_mutex_lock(&ctx->lock);
		info->present = true;
		osal_strncpy(info->name, ctx->config->name,
			     sizeof(info->name));
		info->name[sizeof(info->name) - 1] = '\0';
		info->control = ctx->config->control;
		info->enabled = ctx->enabled;
		info->brightness = ctx->brightness;
		info->max_brightness = ctx->config->max_brightness;
		osal_mutex_unlock(&ctx->lock);
	}
	osal_mutex_unlock(&g_led_registry_lock);

	return OSAL_SUCCESS;
}

static void pdm_led_remove_index(uint32_t index)
{
	pdm_led_context_t *ctx;

	if (!g_led_registry_ready ||
	    index >= OSAL_ARRAY_SIZE(g_led_contexts))
		return;

	osal_mutex_lock(&g_led_registry_lock);
	ctx = g_led_contexts[index];
	g_led_contexts[index] = NULL;
	osal_mutex_unlock(&g_led_registry_lock);

	if (!ctx)
		return;

	pdm_led_deinit_hw(ctx);
	if (ctx->lock_ready)
		osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
}

void pdm_led_remove(const lpf_device_t *device)
{
	if (!device || device->config.type != LPF_DEVICE_TYPE_LED)
		return;

	pdm_led_remove_index(device->config.index);
}

static void pdm_led_registry_deinit(void)
{
	uint32_t i;

	if (!g_led_registry_ready)
		return;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_led_contexts); i++)
		pdm_led_remove_index(i);

	osal_mutex_destroy(&g_led_registry_lock);
	g_led_registry_ready = false;
}

int32_t pdm_led_set_brightness(pdm_led_handle_t handle, uint32_t brightness)
{
	pdm_led_context_t *ctx = handle;
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ctx->brightness = pdm_led_clamp_brightness(ctx, brightness);
	ctx->enabled = ctx->brightness > 0;
	ret = pdm_led_apply(ctx);
	osal_mutex_unlock(&ctx->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pdm_led_set_brightness);

int32_t pdm_led_get_state(pdm_led_handle_t handle, pdm_led_state_t *state)
{
	pdm_led_context_t *ctx = handle;

	if (!ctx || !state)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	state->brightness = ctx->brightness;
	state->max_brightness = ctx->config->max_brightness;
	state->enabled = ctx->enabled;
	osal_mutex_unlock(&ctx->lock);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(pdm_led_get_state);

int32_t pdm_led_enable(pdm_led_handle_t handle)
{
	pdm_led_context_t *ctx = handle;
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ctx->enabled = true;
	if (ctx->brightness == 0)
		ctx->brightness = ctx->config->max_brightness;
	ret = pdm_led_apply(ctx);
	osal_mutex_unlock(&ctx->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pdm_led_enable);

int32_t pdm_led_disable(pdm_led_handle_t handle)
{
	pdm_led_context_t *ctx = handle;
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ctx->enabled = false;
	ret = pdm_led_apply(ctx);
	osal_mutex_unlock(&ctx->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pdm_led_disable);

static int pdm_led_driver_init(void)
{
	int ret;

	ret = pdm_led_chrdev_register();
	if (ret)
		return ret;

	ret = pdm_led_proc_register();
	if (ret) {
		pdm_led_chrdev_unregister();
		return ret;
	}

	LOG_INFO("PDM_LED", "registered");
	return 0;
}

static void pdm_led_driver_exit(void)
{
	pdm_led_registry_deinit();
	pdm_led_proc_unregister();
	pdm_led_chrdev_unregister();
	LOG_INFO("PDM_LED", "unregistered");
}

static const pdm_driver_t g_pdm_led_driver = {
	.name = "led",
	.type = LPF_DEVICE_TYPE_LED,
	.capabilities = LPF_DEVICE_CAP_USER_IOCTL | LPF_DEVICE_CAP_DEBUG_PROCFS,
	.init = pdm_led_driver_init,
	.exit = pdm_led_driver_exit,
	.probe = pdm_led_probe,
	.remove = pdm_led_remove,
};

int32_t pdm_led_driver_register(void)
{
	return lpf_driver_register(&g_pdm_led_driver);
}

void pdm_led_driver_unregister(void)
{
	lpf_driver_unregister(&g_pdm_led_driver);
}
