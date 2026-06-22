// SPDX-License-Identifier: GPL-2.0

#include "osal.h"
#include "lpf/config/lpf_config.h"
#include "lpf/core/lpf_driver.h"
#include "lpf_runtime_internal.h"
#include "lpf_led_internal.h"

static lpf_led_context_t *g_led_context_list;
static osal_mutex_t g_led_registry_lock;
static bool g_led_registry_ready;

static void lpf_led_remove_index(uint32_t index);

static uint32_t lpf_led_clamp_brightness(const lpf_led_context_t *ctx,
					 uint32_t brightness)
{
	if (brightness > ctx->config->max_brightness)
		return ctx->config->max_brightness;

	return brightness;
}

static int32_t lpf_led_apply(lpf_led_context_t *ctx)
{
	const lpf_led_control_ops_t *ops;

	ops = lpf_led_control_get(ctx->config->control);
	if (!ops || !ops->apply)
		return OSAL_ERR_INVALID_PARAM;

	return ops->apply(ctx);
}

static int32_t lpf_led_registry_init(void)
{
	if (g_led_registry_ready)
		return OSAL_SUCCESS;

	if (osal_mutex_init(&g_led_registry_lock, NULL) != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	g_led_registry_ready = true;
	return OSAL_SUCCESS;
}

static void lpf_led_deinit_hw(lpf_led_context_t *ctx)
{
	const lpf_led_control_ops_t *ops;

	if (!ctx)
		return;

	ops = lpf_led_control_get(ctx->config->control);
	if (ops && ops->deinit)
		ops->deinit(ctx);
}

static lpf_led_context_t *lpf_led_find_context_locked(uint32_t index)
{
	lpf_led_context_t *ctx;

	for (ctx = g_led_context_list; ctx; ctx = ctx->next) {
		if (ctx->index == index)
			return ctx;
	}

	return NULL;
}

static void lpf_led_add_context_locked(lpf_led_context_t *ctx)
{
	ctx->next = g_led_context_list;
	g_led_context_list = ctx;
}

static lpf_led_context_t *lpf_led_remove_context_locked(uint32_t index)
{
	lpf_led_context_t **slot = &g_led_context_list;
	lpf_led_context_t *ctx;

	while (*slot) {
		ctx = *slot;
		if (ctx->index == index) {
			*slot = ctx->next;
			ctx->next = NULL;
			return ctx;
		}
		slot = &ctx->next;
	}

	return NULL;
}

static void lpf_led_free_context(lpf_led_context_t *ctx)
{
	if (!ctx)
		return;

	lpf_led_deinit_hw(ctx);
	if (ctx->lock_ready)
		osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
}

static int32_t lpf_led_init_from_entry(uint32_t index,
				       const lpf_config_led_entry_t *entry,
				       lpf_led_handle_t *handle)
{
	lpf_led_context_t *ctx;
	lpf_led_context_t *existing;
	int32_t ret;

	if (!handle || !entry || !entry->enabled)
		return OSAL_ERR_INVALID_PARAM;

	if (entry->config.max_brightness == 0)
		return OSAL_ERR_INVALID_PARAM;

	ret = lpf_led_registry_init();
	if (ret != OSAL_SUCCESS)
		return ret;

	osal_mutex_lock(&g_led_registry_lock);
	existing = lpf_led_find_context_locked(index);
	if (existing) {
		*handle = existing;
		osal_mutex_unlock(&g_led_registry_lock);
		return OSAL_SUCCESS;
	}
	osal_mutex_unlock(&g_led_registry_lock);

	ctx = osal_zalloc(sizeof(*ctx));
	if (!ctx)
		return OSAL_ERR_NO_MEMORY;

	ctx->config = &entry->config;
	ctx->index = index;
	ctx->brightness = lpf_led_clamp_brightness(ctx,
						   entry->config.default_brightness);
	ctx->enabled = ctx->brightness > 0;

	ret = osal_mutex_init(&ctx->lock, NULL);
	if (ret != OSAL_SUCCESS) {
		osal_free(ctx);
		return ret;
	}
	ctx->lock_ready = true;

	{
		const lpf_led_control_ops_t *ops;

		ops = lpf_led_control_get(entry->config.control);
		if (!ops || !ops->init) {
			ret = OSAL_ERR_INVALID_PARAM;
			goto out_free;
		}
		ret = ops->init(ctx);
	}
	if (ret != OSAL_SUCCESS)
		goto out_free;

	ret = lpf_led_apply(ctx);
	if (ret != OSAL_SUCCESS)
		goto out_hw;

	osal_mutex_lock(&g_led_registry_lock);
	existing = lpf_led_find_context_locked(index);
	if (existing) {
		*handle = existing;
		osal_mutex_unlock(&g_led_registry_lock);
		lpf_led_free_context(ctx);
		return OSAL_SUCCESS;
	}
	lpf_led_add_context_locked(ctx);
	osal_mutex_unlock(&g_led_registry_lock);

	*handle = ctx;
	return OSAL_SUCCESS;

out_hw:
	lpf_led_deinit_hw(ctx);
out_free:
	if (ctx->lock_ready)
		osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
	return ret;
}

const lpf_led_control_ops_t *
lpf_led_control_get(lpf_config_led_control_t control)
{
	switch (control) {
	case LPF_CONFIG_LED_CONTROL_GPIO:
		return &lpf_led_gpio_ops;
	case LPF_CONFIG_LED_CONTROL_PWM:
		return &lpf_led_pwm_ops;
	default:
		return NULL;
	}
}

int32_t lpf_led_probe(const lpf_device_t *device)
{
	const lpf_config_led_entry_t *entry;
	lpf_led_handle_t handle = NULL;
	int32_t ret;

	if (!device || device->config.type != LPF_DEVICE_TYPE_LED)
		return OSAL_ERR_INVALID_PARAM;

	entry = (const lpf_config_led_entry_t *)device->config.entry;
	ret = lpf_led_init_from_entry(device->config.index, entry, &handle);
	if (ret != OSAL_SUCCESS)
		return ret;

	ret = lpf_led_chrdev_register_device(device);
	if (ret) {
		lpf_led_remove_index(device->config.index);
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

lpf_led_handle_t lpf_led_get(uint32_t index)
{
	lpf_led_handle_t handle = NULL;

	if (!g_led_registry_ready)
		return NULL;

	osal_mutex_lock(&g_led_registry_lock);
	handle = lpf_led_find_context_locked(index);
	osal_mutex_unlock(&g_led_registry_lock);

	return handle;
}

int32_t lpf_led_debug_get(uint32_t index, lpf_led_debug_info_t *info)
{
	lpf_led_context_t *ctx;

	if (!info)
		return OSAL_ERR_INVALID_PARAM;

	osal_memset(info, 0, sizeof(*info));

	if (!g_led_registry_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_led_registry_lock);
	ctx = lpf_led_find_context_locked(index);
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

static void lpf_led_remove_index(uint32_t index)
{
	lpf_led_context_t *ctx;

	if (!g_led_registry_ready)
		return;

	osal_mutex_lock(&g_led_registry_lock);
	ctx = lpf_led_remove_context_locked(index);
	osal_mutex_unlock(&g_led_registry_lock);

	lpf_led_free_context(ctx);
}

void lpf_led_remove(const lpf_device_t *device)
{
	if (!device || device->config.type != LPF_DEVICE_TYPE_LED)
		return;

	lpf_led_chrdev_unregister_device(device);
	lpf_led_remove_index(device->config.index);
}

static void lpf_led_registry_deinit(void)
{
	lpf_led_context_t *ctx;

	if (!g_led_registry_ready)
		return;

	for (;;) {
		osal_mutex_lock(&g_led_registry_lock);
		ctx = g_led_context_list;
		if (ctx)
			g_led_context_list = ctx->next;
		osal_mutex_unlock(&g_led_registry_lock);

		if (!ctx)
			break;

		ctx->next = NULL;
		lpf_led_free_context(ctx);
	}

	osal_mutex_destroy(&g_led_registry_lock);
	g_led_registry_ready = false;
}

int32_t lpf_led_set_brightness(lpf_led_handle_t handle, uint32_t brightness)
{
	lpf_led_context_t *ctx = handle;
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ctx->brightness = lpf_led_clamp_brightness(ctx, brightness);
	ctx->enabled = ctx->brightness > 0;
	ret = lpf_led_apply(ctx);
	osal_mutex_unlock(&ctx->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(lpf_led_set_brightness);

int32_t lpf_led_get_state(lpf_led_handle_t handle,
			  lpf_led_service_state_t *state)
{
	lpf_led_context_t *ctx = handle;

	if (!ctx || !state)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	state->brightness = ctx->brightness;
	state->max_brightness = ctx->config->max_brightness;
	state->enabled = ctx->enabled;
	osal_mutex_unlock(&ctx->lock);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_led_get_state);

int32_t lpf_led_enable(lpf_led_handle_t handle)
{
	lpf_led_context_t *ctx = handle;
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ctx->enabled = true;
	if (ctx->brightness == 0)
		ctx->brightness = ctx->config->max_brightness;
	ret = lpf_led_apply(ctx);
	osal_mutex_unlock(&ctx->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(lpf_led_enable);

int32_t lpf_led_disable(lpf_led_handle_t handle)
{
	lpf_led_context_t *ctx = handle;
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ctx->enabled = false;
	ret = lpf_led_apply(ctx);
	osal_mutex_unlock(&ctx->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(lpf_led_disable);

static int lpf_led_driver_init(void)
{
	int ret;

	ret = lpf_led_chrdev_register();
	if (ret)
		return ret;

	ret = lpf_led_proc_register();
	if (ret) {
		lpf_led_chrdev_unregister();
		return ret;
	}

	ret = lpf_led_debugfs_register();
	if (ret) {
		lpf_led_proc_unregister();
		lpf_led_chrdev_unregister();
		return ret;
	}

	LOG_INFO("LPF_LED", "registered");
	return 0;
}

static void lpf_led_driver_exit(void)
{
	lpf_led_registry_deinit();
	lpf_led_debugfs_unregister();
	lpf_led_proc_unregister();
	lpf_led_chrdev_unregister();
	LOG_INFO("LPF_LED", "unregistered");
}

static const lpf_driver_t g_lpf_led_driver = {
	.name = "led",
	.type = LPF_DEVICE_TYPE_LED,
	.capabilities = LPF_DEVICE_CAP_USER_IOCTL | LPF_DEVICE_CAP_DEBUGFS,
	.init = lpf_led_driver_init,
	.exit = lpf_led_driver_exit,
	.probe = lpf_led_probe,
	.remove = lpf_led_remove,
};

static int32_t lpf_led_service_register(void)
{
	return lpf_driver_register(&g_lpf_led_driver);
}

static void lpf_led_service_unregister(void)
{
	lpf_driver_unregister(&g_lpf_led_driver);
}

lpf_runtime_service_register(led_service, lpf_led_service_register,
			     lpf_led_service_unregister);
