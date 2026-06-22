// SPDX-License-Identifier: GPL-2.0

#include "osal.h"
#include "pdm/config/pdm_config.h"
#include "pdm/core/pdm_driver.h"
#include "pdm_runtime_internal.h"
#include "pdm_led_internal.h"

static pdm_led_context_t *g_led_context_list;
static osal_mutex_t g_led_registry_lock;
static bool g_led_registry_ready;

static void pdm_led_remove_index(uint32_t index);

static uint32_t pdm_led_clamp_brightness(const pdm_led_context_t *ctx,
					 uint32_t brightness)
{
	if (brightness > ctx->config->max_brightness)
		return ctx->config->max_brightness;

	return brightness;
}

static int32_t pdm_led_apply(pdm_led_context_t *ctx)
{
	const pdm_led_control_ops_t *ops;

	ops = pdm_led_control_get(ctx->config->control);
	if (!ops || !ops->apply)
		return OSAL_ERR_INVALID_PARAM;

	return ops->apply(ctx);
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

static void pdm_led_deinit_hw(pdm_led_context_t *ctx)
{
	const pdm_led_control_ops_t *ops;

	if (!ctx)
		return;

	ops = pdm_led_control_get(ctx->config->control);
	if (ops && ops->deinit)
		ops->deinit(ctx);
}

static pdm_led_context_t *pdm_led_find_context_locked(uint32_t index)
{
	pdm_led_context_t *ctx;

	for (ctx = g_led_context_list; ctx; ctx = ctx->next) {
		if (ctx->index == index)
			return ctx;
	}

	return NULL;
}

static void pdm_led_add_context_locked(pdm_led_context_t *ctx)
{
	ctx->next = g_led_context_list;
	g_led_context_list = ctx;
}

static pdm_led_context_t *pdm_led_remove_context_locked(uint32_t index)
{
	pdm_led_context_t **slot = &g_led_context_list;
	pdm_led_context_t *ctx;

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

static void pdm_led_free_context(pdm_led_context_t *ctx)
{
	if (!ctx)
		return;

	pdm_led_deinit_hw(ctx);
	if (ctx->lock_ready)
		osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
}

static int32_t pdm_led_init_from_entry(uint32_t index,
				       const pdm_config_led_entry_t *entry,
				       pdm_led_handle_t *handle)
{
	pdm_led_context_t *ctx;
	pdm_led_context_t *existing;
	int32_t ret;

	if (!handle || !entry || !entry->enabled)
		return OSAL_ERR_INVALID_PARAM;

	if (entry->config.max_brightness == 0)
		return OSAL_ERR_INVALID_PARAM;

	ret = pdm_led_registry_init();
	if (ret != OSAL_SUCCESS)
		return ret;

	osal_mutex_lock(&g_led_registry_lock);
	existing = pdm_led_find_context_locked(index);
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
	ctx->brightness = pdm_led_clamp_brightness(ctx,
						   entry->config.default_brightness);
	ctx->enabled = ctx->brightness > 0;

	ret = osal_mutex_init(&ctx->lock, NULL);
	if (ret != OSAL_SUCCESS) {
		osal_free(ctx);
		return ret;
	}
	ctx->lock_ready = true;

	{
		const pdm_led_control_ops_t *ops;

		ops = pdm_led_control_get(entry->config.control);
		if (!ops || !ops->init) {
			ret = OSAL_ERR_INVALID_PARAM;
			goto out_free;
		}
		ret = ops->init(ctx);
	}
	if (ret != OSAL_SUCCESS)
		goto out_free;

	ret = pdm_led_apply(ctx);
	if (ret != OSAL_SUCCESS)
		goto out_hw;

	osal_mutex_lock(&g_led_registry_lock);
	existing = pdm_led_find_context_locked(index);
	if (existing) {
		*handle = existing;
		osal_mutex_unlock(&g_led_registry_lock);
		pdm_led_free_context(ctx);
		return OSAL_SUCCESS;
	}
	pdm_led_add_context_locked(ctx);
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
}

const pdm_led_control_ops_t *
pdm_led_control_get(pdm_config_led_control_t control)
{
	switch (control) {
	case PDM_CONFIG_LED_CONTROL_GPIO:
		return &pdm_led_gpio_ops;
	case PDM_CONFIG_LED_CONTROL_PWM:
		return &pdm_led_pwm_ops;
	default:
		return NULL;
	}
}

int32_t pdm_led_probe(const pdm_device_t *device)
{
	const pdm_config_led_entry_t *entry;
	pdm_led_handle_t handle = NULL;
	int32_t ret;

	if (!device || device->config.type != PDM_DEVICE_TYPE_LED)
		return OSAL_ERR_INVALID_PARAM;

	entry = (const pdm_config_led_entry_t *)device->config.entry;
	ret = pdm_led_init_from_entry(device->config.index, entry, &handle);
	if (ret != OSAL_SUCCESS)
		return ret;

	ret = pdm_led_chrdev_register_device(device);
	if (ret) {
		pdm_led_remove_index(device->config.index);
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

pdm_led_handle_t pdm_led_get(uint32_t index)
{
	pdm_led_handle_t handle = NULL;

	if (!g_led_registry_ready)
		return NULL;

	osal_mutex_lock(&g_led_registry_lock);
	handle = pdm_led_find_context_locked(index);
	osal_mutex_unlock(&g_led_registry_lock);

	return handle;
}

int32_t pdm_led_debug_get(uint32_t index, pdm_led_debug_info_t *info)
{
	pdm_led_context_t *ctx;

	if (!info)
		return OSAL_ERR_INVALID_PARAM;

	osal_memset(info, 0, sizeof(*info));

	if (!g_led_registry_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_led_registry_lock);
	ctx = pdm_led_find_context_locked(index);
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

	if (!g_led_registry_ready)
		return;

	osal_mutex_lock(&g_led_registry_lock);
	ctx = pdm_led_remove_context_locked(index);
	osal_mutex_unlock(&g_led_registry_lock);

	pdm_led_free_context(ctx);
}

void pdm_led_remove(const pdm_device_t *device)
{
	if (!device || device->config.type != PDM_DEVICE_TYPE_LED)
		return;

	pdm_led_chrdev_unregister_device(device);
	pdm_led_remove_index(device->config.index);
}

static void pdm_led_registry_deinit(void)
{
	pdm_led_context_t *ctx;

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
		pdm_led_free_context(ctx);
	}

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

int32_t pdm_led_get_state(pdm_led_handle_t handle,
			  pdm_led_service_state_t *state)
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

	ret = pdm_led_debugfs_register();
	if (ret) {
		pdm_led_proc_unregister();
		pdm_led_chrdev_unregister();
		return ret;
	}

	LOG_INFO("PDM_LED", "registered");
	return 0;
}

static void pdm_led_driver_exit(void)
{
	pdm_led_registry_deinit();
	pdm_led_debugfs_unregister();
	pdm_led_proc_unregister();
	pdm_led_chrdev_unregister();
	LOG_INFO("PDM_LED", "unregistered");
}

static const pdm_driver_t g_lpf_led_driver = {
	.name = "led",
	.type = PDM_DEVICE_TYPE_LED,
	.capabilities = PDM_DEVICE_CAP_USER_IOCTL | PDM_DEVICE_CAP_DEBUGFS,
	.init = pdm_led_driver_init,
	.exit = pdm_led_driver_exit,
	.probe = pdm_led_probe,
	.remove = pdm_led_remove,
};

static int32_t pdm_led_service_register(void)
{
	return pdm_driver_register(&g_lpf_led_driver);
}

static void pdm_led_service_unregister(void)
{
	pdm_driver_unregister(&g_lpf_led_driver);
}

pdm_runtime_service_register(led_service, pdm_led_service_register,
			     pdm_led_service_unregister);

#ifdef CONFIG_PDM_NEW_BUS
/*===========================================================================
 * 新总线驱动注册（Linux bus_type）
 *===========================================================================*/

#include "pdm/core/pdm_bus.h"
#include "pdm/core/pdm_device_new.h"

/**
 * @brief 新总线的 probe 函数
 */
static int pdm_led_probe_new(struct pdm_device *pdm_dev)
{
	struct device_node *np = pdm_dev->dev.of_node;
	pdm_config_led_entry_t *entry;
	pdm_led_handle_t handle = NULL;
	const char *control_str;
	const char *name_str;
	u32 value;
	int ret;

	if (!np) {
		dev_err(&pdm_dev->dev, "No device tree node\n");
		return -EINVAL;
	}

	/* 分配配置结构 */
	entry = devm_kzalloc(&pdm_dev->dev, sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	entry->enabled = true;

	/* 读取基本配置 */
	if (of_property_read_string(np, "label", &entry->description))
		entry->description = "PDM LED Device";

	if (of_property_read_string(np, "pdm,name", &name_str))
		name_str = dev_name(&pdm_dev->dev);
	snprintf(entry->config.name, sizeof(entry->config.name), "%s", name_str);

	/* 读取控制类型 */
	if (of_property_read_string(np, "pdm,control", &control_str)) {
		dev_err(&pdm_dev->dev, "Missing pdm,control property\n");
		return -EINVAL;
	}

	if (strcmp(control_str, "gpio") == 0) {
		entry->config.control = PDM_CONFIG_LED_CONTROL_GPIO;

		/* 读取 GPIO 配置 */
		if (of_property_read_string(np, "pdm,gpio-name",
					    &entry->config.hw.gpio.consumer)) {
			dev_err(&pdm_dev->dev, "Missing pdm,gpio-name property\n");
			return -EINVAL;
		}

		entry->config.hw.gpio.polarity_inversed =
			of_property_read_bool(np, "pdm,gpio-active-low");

	} else if (strcmp(control_str, "pwm") == 0) {
		entry->config.control = PDM_CONFIG_LED_CONTROL_PWM;

		/* 读取 PWM 配置 */
		if (of_property_read_string(np, "pdm,pwm-name",
					    &entry->config.hw.pwm.consumer)) {
			dev_err(&pdm_dev->dev, "Missing pdm,pwm-name property\n");
			return -EINVAL;
		}

		if (!of_property_read_u32(np, "pdm,pwm-period-ns", &value))
			entry->config.hw.pwm.period_ns = value;
		else
			entry->config.hw.pwm.period_ns = 1000000; /* 1kHz 默认 */

		entry->config.hw.pwm.polarity_inversed =
			of_property_read_bool(np, "pdm,pwm-inverted");

	} else {
		dev_err(&pdm_dev->dev, "Unknown control type: %s\n", control_str);
		return -EINVAL;
	}

	/* 读取亮度配置 */
	if (!of_property_read_u32(np, "pdm,max-brightness", &value))
		entry->config.max_brightness = value;
	else
		entry->config.max_brightness = 255;

	if (!of_property_read_u32(np, "pdm,default-brightness", &value))
		entry->config.default_brightness = value;
	else
		entry->config.default_brightness = 0;

	/* 初始化 LED 上下文 */
	ret = pdm_led_init_from_entry(pdm_dev->id, entry, &handle);
	if (ret != OSAL_SUCCESS) {
		dev_err(&pdm_dev->dev, "Failed to initialize LED: %d\n", ret);
		return -EIO;
	}

	/* 创建字符设备 */
	{
		pdm_device_t temp_device = {
			.config = {
				.type = PDM_DEVICE_TYPE_LED,
				.index = pdm_dev->id,
				.entry = entry
			}
		};
		ret = pdm_led_chrdev_register_device(&temp_device);
		if (ret) {
			dev_err(&pdm_dev->dev, "Failed to register chardev: %d\n", ret);
			pdm_led_remove_index(pdm_dev->id);
			return ret;
		}
	}

	/* 保存上下文 */
	pdm_device_set_drvdata(pdm_dev, handle);

	dev_info(&pdm_dev->dev, "LED device probed successfully (control=%s, id=%d)\n",
		 control_str, pdm_dev->id);

	return 0;
}

/**
 * @brief 新总线的 remove 函数
 */
static void pdm_led_remove_new(struct pdm_device *pdm_dev)
{
	pdm_device_t temp_device = {
		.config = {
			.type = PDM_DEVICE_TYPE_LED,
			.index = pdm_dev->id,
		}
	};

	dev_info(&pdm_dev->dev, "Removing LED device (id=%d)\n", pdm_dev->id);

	/* 注销字符设备 */
	pdm_led_chrdev_unregister_device(&temp_device);

	/* 清理 LED 上下文 */
	pdm_led_remove_index(pdm_dev->id);
}

/**
 * @brief Device Tree 匹配表
 */
static const struct of_device_id pdm_led_of_match[] = {
	{ .compatible = "vendor,pdm-led-gpio" },
	{ .compatible = "vendor,pdm-led-pwm" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_led_of_match);

/**
 * @brief PDM LED 驱动结构（新总线）
 */
static struct pdm_driver pdm_led_driver_new = {
	.driver = {
		.name = "pdm-led",
		.of_match_table = pdm_led_of_match,
	},
	.probe = pdm_led_probe_new,
	.remove = pdm_led_remove_new,
};

/**
 * @brief 模块初始化（新总线）
 */
static int __init pdm_led_driver_init_new(void)
{
	int ret;

	LOG_INFO("PDM_LED", "Registering LED driver on new PDM bus");

	ret = pdm_driver_register(&pdm_led_driver_new);
	if (ret) {
		LOG_ERROR("PDM_LED", "Failed to register driver, error %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief 模块退出（新总线）
 */
static void __exit pdm_led_driver_exit_new(void)
{
	pdm_bus_unregister_driver(&pdm_led_driver_new);
	LOG_INFO("PDM_LED", "LED driver unregistered from PDM bus");
}

module_init(pdm_led_driver_init_new);
module_exit(pdm_led_driver_exit_new);

#endif /* CONFIG_PDM_NEW_BUS */
