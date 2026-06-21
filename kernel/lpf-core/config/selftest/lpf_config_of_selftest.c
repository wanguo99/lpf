// SPDX-License-Identifier: GPL-2.0

#include "osal.h"
#include "lpf/config/lpf_config.h"
#include "lpf_runtime_internal.h"
#include "lpf_config_dt_parser.h"
#include "lpf_config_normalizer.h"

#define LPF_CONFIG_OF_SELFTEST_DEVICE_CAPACITY 8U

#define LPF_CONFIG_OF_SELFTEST_EXPECT(condition, message) \
	do { \
		if (!(condition)) \
			return lpf_config_of_selftest_fail(message); \
	} while (0)

#if IS_ENABLED(CONFIG_OF)
#include <linux/of.h>

#if IS_ENABLED(CONFIG_OF_DYNAMIC)
#define LPF_CONFIG_OF_SELFTEST_CAN_USE_CHANGESET 1
#else
#define LPF_CONFIG_OF_SELFTEST_CAN_USE_CHANGESET 0
#endif

static const lpf_config_mcu_entry_t g_lpf_config_of_selftest_mcu0 = {
	.description = "Mock CAN MCU",
	.enabled = true,
	.config = {
		.name = "mcu0",
		.interface = LPF_CONFIG_MCU_INTERFACE_CAN,
		.hw.can = {
			.device = "mock-can0",
			.bitrate = 500000U,
			.rx_timeout = 10U,
			.tx_timeout = 10U,
			.tx_id = 0x123U,
			.rx_id = 0x123U,
		},
		.cmd_timeout_ms = 1000U,
		.retry_count = 0U,
	},
	.reset_gpio = NULL,
	.irq_gpio = NULL,
};

static const lpf_config_led_entry_t g_lpf_config_of_selftest_leds[] = {
	{
		.description = "Mock GPIO status LED",
		.enabled = true,
		.config = {
			.name = "status",
			.control = LPF_CONFIG_LED_CONTROL_GPIO,
			.max_brightness = 1U,
			.default_brightness = 0U,
			.hw.gpio = {
				.gpio_num = 8U,
				.pin_mux = 0U,
				.active_low = false,
				.pull_up = false,
				.pull_down = false,
			},
		},
	},
	{
		.description = "Mock PWM activity LED",
		.enabled = true,
		.config = {
			.name = "activity",
			.control = LPF_CONFIG_LED_CONTROL_PWM,
			.max_brightness = 255U,
			.default_brightness = 0U,
			.hw.pwm = {
				.consumer = "lpf-mock-led-pwm0",
				.period_ns = 1000000U,
				.polarity_inversed = false,
			},
		},
	},
};

static const lpf_config_platform_config_t g_lpf_config_of_selftest_expected = {
	.platform_name = "linux",
	.chip_name = "x86_64",
	.project_name = "x86_mock_modules",
	.product_name = "ubuntu",
	.version = "1.0.0",
	.mcu_count = 1U,
	.mcu_array = &g_lpf_config_of_selftest_mcu0,
	.led_count = OSAL_ARRAY_SIZE(g_lpf_config_of_selftest_leds),
	.led_array = g_lpf_config_of_selftest_leds,
};

static int32_t lpf_config_of_selftest_fail(const char *message)
{
	pr_err("LPF:CONFIG_OF_SELFTEST: %s\n", message);
	return OSAL_ERR_GENERIC;
}

static const char *lpf_config_of_selftest_name(const void *node)
{
	const struct device_node *of_node = node;

	return of_node ? of_node->name : NULL;
}

static const void *lpf_config_of_selftest_next_available_child(
	const void *node, const void *previous)
{
	return of_get_next_available_child((const struct device_node *)node,
					   (struct device_node *)previous);
}

static void lpf_config_of_selftest_put_node(const void *node)
{
	of_node_put((struct device_node *)node);
}

static int32_t lpf_config_of_selftest_read_string(
	const void *node, const char *property, const char **value)
{
	if (of_property_read_string((const struct device_node *)node, property,
				    value) != 0)
		return OSAL_ERR_NAME_NOT_FOUND;

	return OSAL_SUCCESS;
}

static bool lpf_config_of_selftest_read_bool(const void *node,
					     const char *property)
{
	return of_property_read_bool((const struct device_node *)node, property);
}

static int32_t lpf_config_of_selftest_read_u32(const void *node,
					       const char *property,
					       uint32_t *value)
{
	if (of_property_read_u32((const struct device_node *)node, property,
				 value) != 0)
		return OSAL_ERR_NAME_NOT_FOUND;

	return OSAL_SUCCESS;
}

static const lpf_config_dt_node_ops_t g_lpf_config_of_selftest_ops = {
	.name = lpf_config_of_selftest_name,
	.next_available_child = lpf_config_of_selftest_next_available_child,
	.put_node = lpf_config_of_selftest_put_node,
	.read_string = lpf_config_of_selftest_read_string,
	.read_bool = lpf_config_of_selftest_read_bool,
	.read_u32 = lpf_config_of_selftest_read_u32,
	.alloc = osal_zalloc,
	.free = osal_free,
};

static int32_t lpf_config_of_selftest_string_equal(const char *left,
						   const char *right)
{
	if (!left || !right)
		return left == right ? OSAL_SUCCESS : OSAL_ERR_GENERIC;

	return osal_strcmp(left, right) == 0 ? OSAL_SUCCESS :
					       OSAL_ERR_GENERIC;
}

static int32_t lpf_config_of_selftest_compare_mcu(
	const lpf_config_mcu_entry_t *left,
	const lpf_config_mcu_entry_t *right)
{
	const lpf_config_mcu_config_t *a;
	const lpf_config_mcu_config_t *b;

	if (!left || !right)
		return OSAL_ERR_INVALID_PARAM;

	a = &left->config;
	b = &right->config;
	if (left->enabled != right->enabled)
		return OSAL_ERR_GENERIC;
	if (lpf_config_of_selftest_string_equal(a->name, b->name) !=
	    OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;
	if (a->interface != b->interface ||
	    a->cmd_timeout_ms != b->cmd_timeout_ms ||
	    a->retry_count != b->retry_count)
		return OSAL_ERR_GENERIC;

	switch (a->interface) {
	case LPF_CONFIG_MCU_INTERFACE_CAN:
		if (lpf_config_of_selftest_string_equal(a->hw.can.device,
							b->hw.can.device) !=
		    OSAL_SUCCESS)
			return OSAL_ERR_GENERIC;
		return a->hw.can.bitrate == b->hw.can.bitrate &&
			       a->hw.can.rx_timeout == b->hw.can.rx_timeout &&
			       a->hw.can.tx_timeout == b->hw.can.tx_timeout &&
			       a->hw.can.tx_id == b->hw.can.tx_id &&
			       a->hw.can.rx_id == b->hw.can.rx_id ?
			       OSAL_SUCCESS :
			       OSAL_ERR_GENERIC;
	case LPF_CONFIG_MCU_INTERFACE_SERIAL:
		if (lpf_config_of_selftest_string_equal(a->hw.serial.device,
							b->hw.serial.device) !=
		    OSAL_SUCCESS)
			return OSAL_ERR_GENERIC;
		return a->hw.serial.baudrate == b->hw.serial.baudrate &&
			       a->hw.serial.data_bits == b->hw.serial.data_bits &&
			       a->hw.serial.stop_bits == b->hw.serial.stop_bits &&
			       a->hw.serial.parity == b->hw.serial.parity &&
			       a->hw.serial.flow_control ==
				       b->hw.serial.flow_control ?
			       OSAL_SUCCESS :
			       OSAL_ERR_GENERIC;
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}
}

static int32_t lpf_config_of_selftest_compare_led(
	const lpf_config_led_entry_t *left,
	const lpf_config_led_entry_t *right)
{
	const lpf_config_led_config_t *a;
	const lpf_config_led_config_t *b;

	if (!left || !right)
		return OSAL_ERR_INVALID_PARAM;

	a = &left->config;
	b = &right->config;
	if (left->enabled != right->enabled)
		return OSAL_ERR_GENERIC;
	if (lpf_config_of_selftest_string_equal(a->name, b->name) !=
	    OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;
	if (a->control != b->control ||
	    a->max_brightness != b->max_brightness ||
	    a->default_brightness != b->default_brightness)
		return OSAL_ERR_GENERIC;

	switch (a->control) {
	case LPF_CONFIG_LED_CONTROL_GPIO:
		return a->hw.gpio.gpio_num == b->hw.gpio.gpio_num &&
			       a->hw.gpio.pin_mux == b->hw.gpio.pin_mux &&
			       a->hw.gpio.active_low == b->hw.gpio.active_low &&
			       a->hw.gpio.pull_up == b->hw.gpio.pull_up &&
			       a->hw.gpio.pull_down == b->hw.gpio.pull_down ?
			       OSAL_SUCCESS :
			       OSAL_ERR_GENERIC;
	case LPF_CONFIG_LED_CONTROL_PWM:
		if (lpf_config_of_selftest_string_equal(a->hw.pwm.consumer,
							b->hw.pwm.consumer) !=
		    OSAL_SUCCESS)
			return OSAL_ERR_GENERIC;
		return a->hw.pwm.period_ns == b->hw.pwm.period_ns &&
			       a->hw.pwm.polarity_inversed ==
				       b->hw.pwm.polarity_inversed ?
			       OSAL_SUCCESS :
			       OSAL_ERR_GENERIC;
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}
}

static int32_t lpf_config_of_selftest_compare_device(
	const lpf_config_device_config_t *left,
	const lpf_config_device_config_t *right)
{
	if (!left || !right)
		return OSAL_ERR_INVALID_PARAM;

	if (left->device_type != right->device_type || left->index != right->index)
		return OSAL_ERR_GENERIC;

	switch (left->device_type) {
	case LPF_CONFIG_DEVICE_TYPE_MCU:
		return lpf_config_of_selftest_compare_mcu(left->entry,
							  right->entry);
	case LPF_CONFIG_DEVICE_TYPE_LED:
		return lpf_config_of_selftest_compare_led(left->entry,
							  right->entry);
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}
}

static int32_t lpf_config_of_selftest_normalize(
	const lpf_config_platform_config_t *platform,
	lpf_config_device_config_t *devices, uint32_t *count)
{
	*count = LPF_CONFIG_OF_SELFTEST_DEVICE_CAPACITY;
	return lpf_config_normalize_devices(platform, devices, count);
}

static int32_t lpf_config_of_selftest_expect_platform(
	const lpf_config_platform_config_t *platform)
{
	LPF_CONFIG_OF_SELFTEST_EXPECT(platform != NULL, "platform is NULL");
	LPF_CONFIG_OF_SELFTEST_EXPECT(
		lpf_config_of_selftest_string_equal(platform->platform_name,
						    "linux") == OSAL_SUCCESS,
		"platform-name mismatch");
	LPF_CONFIG_OF_SELFTEST_EXPECT(
		lpf_config_of_selftest_string_equal(platform->chip_name,
						    "x86_64") == OSAL_SUCCESS,
		"chip-name mismatch");
	LPF_CONFIG_OF_SELFTEST_EXPECT(
		lpf_config_of_selftest_string_equal(platform->project_name,
						    "x86_mock_modules") ==
			OSAL_SUCCESS,
		"project-name mismatch");
	LPF_CONFIG_OF_SELFTEST_EXPECT(
		lpf_config_of_selftest_string_equal(platform->product_name,
						    "ubuntu") == OSAL_SUCCESS,
		"product-name mismatch");
	LPF_CONFIG_OF_SELFTEST_EXPECT(
		lpf_config_of_selftest_string_equal(platform->version,
						    "1.0.0") == OSAL_SUCCESS,
		"config-version mismatch");
	LPF_CONFIG_OF_SELFTEST_EXPECT(platform->mcu_count == 1U,
				      "mcu count mismatch");
	LPF_CONFIG_OF_SELFTEST_EXPECT(platform->led_count == 2U,
				      "led count mismatch");
	return OSAL_SUCCESS;
}

static int32_t lpf_config_of_selftest_compare_to_static(
	const lpf_config_platform_config_t *platform)
{
	lpf_config_device_config_t
		static_devices[LPF_CONFIG_OF_SELFTEST_DEVICE_CAPACITY];
	lpf_config_device_config_t
		of_devices[LPF_CONFIG_OF_SELFTEST_DEVICE_CAPACITY];
	uint32_t static_count;
	uint32_t of_count;
	uint32_t i;
	int32_t ret;

	ret = lpf_config_of_selftest_normalize(
		&g_lpf_config_of_selftest_expected, static_devices,
		&static_count);
	if (ret != OSAL_SUCCESS)
		return ret;

	ret = lpf_config_of_selftest_normalize(platform, of_devices, &of_count);
	if (ret != OSAL_SUCCESS)
		return ret;

	LPF_CONFIG_OF_SELFTEST_EXPECT(static_count == of_count,
				      "normalized count mismatch");
	LPF_CONFIG_OF_SELFTEST_EXPECT(static_count == 3U,
				      "unexpected normalized count");

	for (i = 0; i < static_count; i++) {
		ret = lpf_config_of_selftest_compare_device(&static_devices[i],
							    &of_devices[i]);
		if (ret != OSAL_SUCCESS) {
			pr_err("LPF:CONFIG_OF_SELFTEST: device %u mismatch\n", i);
			return ret;
		}
	}

	return OSAL_SUCCESS;
}

#if LPF_CONFIG_OF_SELFTEST_CAN_USE_CHANGESET
static struct device_node *
lpf_config_of_selftest_create_node(struct of_changeset *changeset,
				   struct device_node *parent,
				   const char *name)
{
	struct device_node *node;

	node = of_changeset_create_node(changeset, parent, name);
	if (IS_ERR(node))
		return node;

	of_node_set_flag(node, OF_POPULATED);
	return node;
}

static int lpf_config_of_selftest_add_root_props(struct of_changeset *changeset,
						 struct device_node *root)
{
	int ret;

	ret = of_changeset_add_prop_string(changeset, root, "compatible",
					   "lpf,config-selftest");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, root, "platform-name",
					   "linux");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, root, "chip-name",
					   "x86_64");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, root, "project-name",
					   "x86_mock_modules");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, root, "product-name",
					   "ubuntu");
	if (ret != 0)
		return ret;
	return of_changeset_add_prop_string(changeset, root, "config-version",
					    "1.0.0");
}

static int lpf_config_of_selftest_add_mcu_props(struct of_changeset *changeset,
						struct device_node *mcu)
{
	int ret;

	ret = of_changeset_add_prop_string(changeset, mcu, "label", "mcu0");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, mcu, "interface", "can");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, mcu, "device",
					   "mock-can0");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, mcu, "bitrate", 500000U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, mcu, "rx-timeout-ms", 10U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, mcu, "tx-timeout-ms", 10U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, mcu, "tx-id", 0x123U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, mcu, "rx-id", 0x123U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, mcu, "cmd-timeout-ms",
					1000U);
	if (ret != 0)
		return ret;
	return of_changeset_add_prop_u32(changeset, mcu, "retry-count", 0U);
}

static int lpf_config_of_selftest_add_status_led_props(
	struct of_changeset *changeset, struct device_node *led)
{
	int ret;

	ret = of_changeset_add_prop_string(changeset, led, "label", "status");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, led, "control", "gpio");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, led, "max-brightness", 1U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, led, "default-brightness",
					0U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, led, "gpio", 8U);
	if (ret != 0)
		return ret;
	return of_changeset_add_prop_u32(changeset, led, "pin-mux", 0U);
}

static int lpf_config_of_selftest_add_activity_led_props(
	struct of_changeset *changeset, struct device_node *led)
{
	int ret;

	ret = of_changeset_add_prop_string(changeset, led, "label", "activity");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, led, "control", "pwm");
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, led, "max-brightness",
					255U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_u32(changeset, led, "default-brightness",
					0U);
	if (ret != 0)
		return ret;
	ret = of_changeset_add_prop_string(changeset, led, "consumer",
					   "lpf-mock-led-pwm0");
	if (ret != 0)
		return ret;
	return of_changeset_add_prop_u32(changeset, led, "period-ns",
					 1000000U);
}

static int32_t lpf_config_of_selftest_add_overlay(
	struct of_changeset *changeset, struct device_node **root_out)
{
	struct device_node *lpf;
	struct device_node *mcu_group;
	struct device_node *led_group;
	struct device_node *mcu;
	struct device_node *status_led;
	struct device_node *activity_led;
	int ret;

	if (!of_root)
		return OSAL_ERR_NOT_SUPPORTED;

	lpf = lpf_config_of_selftest_create_node(changeset, of_root,
						 "lpf-config-selftest");
	if (IS_ERR(lpf))
		return lpf_config_of_selftest_fail("failed to create lpf node");

	ret = lpf_config_of_selftest_add_root_props(changeset, lpf);
	if (ret != 0)
		return OSAL_ERR_GENERIC;

	mcu_group = lpf_config_of_selftest_create_node(changeset, lpf, "mcu");
	if (IS_ERR(mcu_group))
		return OSAL_ERR_GENERIC;
	led_group = lpf_config_of_selftest_create_node(changeset, lpf, "led");
	if (IS_ERR(led_group))
		return OSAL_ERR_GENERIC;

	mcu = lpf_config_of_selftest_create_node(changeset, mcu_group, "mcu0");
	if (IS_ERR(mcu))
		return OSAL_ERR_GENERIC;
	ret = lpf_config_of_selftest_add_mcu_props(changeset, mcu);
	if (ret != 0)
		return OSAL_ERR_GENERIC;

	status_led = lpf_config_of_selftest_create_node(changeset, led_group,
						       "status");
	if (IS_ERR(status_led))
		return OSAL_ERR_GENERIC;
	ret = lpf_config_of_selftest_add_status_led_props(changeset,
							  status_led);
	if (ret != 0)
		return OSAL_ERR_GENERIC;

	activity_led = lpf_config_of_selftest_create_node(changeset, led_group,
							 "activity");
	if (IS_ERR(activity_led))
		return OSAL_ERR_GENERIC;
	ret = lpf_config_of_selftest_add_activity_led_props(changeset,
							    activity_led);
	if (ret != 0)
		return OSAL_ERR_GENERIC;

	ret = of_changeset_apply(changeset);
	if (ret != 0)
		return OSAL_ERR_GENERIC;

	*root_out = of_find_node_by_path("/lpf-config-selftest");
	return *root_out ? OSAL_SUCCESS : OSAL_ERR_NAME_NOT_FOUND;
}

static int32_t lpf_config_of_selftest_run(void)
{
	lpf_config_dt_parse_result_t parsed;
	struct of_changeset changeset;
	struct device_node *root = NULL;
	int32_t ret;
	int revert_ret = 0;

	of_changeset_init(&changeset);
	ret = lpf_config_of_selftest_add_overlay(&changeset, &root);
	if (ret != OSAL_SUCCESS)
		goto out_destroy;

	ret = lpf_config_dt_parse_platform(&g_lpf_config_of_selftest_ops, root,
					   &parsed);
	of_node_put(root);
	root = NULL;
	if (ret != OSAL_SUCCESS)
		goto out_revert;

	ret = lpf_config_of_selftest_expect_platform(&parsed.platform);
	if (ret == OSAL_SUCCESS)
		ret = lpf_config_of_selftest_compare_to_static(&parsed.platform);
	lpf_config_dt_parse_result_clear(&g_lpf_config_of_selftest_ops,
					 &parsed);

out_revert:
	revert_ret = of_changeset_revert(&changeset);
out_destroy:
	of_changeset_destroy(&changeset);
	if (ret == OSAL_SUCCESS && revert_ret != 0)
		ret = lpf_config_of_selftest_fail("failed to revert OF overlay");
	return ret;
}
#else
static int32_t lpf_config_of_selftest_run(void)
{
	pr_info("LPF:CONFIG_OF_SELFTEST: CONFIG_OF_DYNAMIC unavailable, skipping live OF overlay checks\n");
	return OSAL_SUCCESS;
}
#endif

#else
static int32_t lpf_config_of_selftest_run(void)
{
	pr_info("LPF:CONFIG_OF_SELFTEST: CONFIG_OF unavailable, skipping live OF checks\n");
	return OSAL_SUCCESS;
}
#endif

static int32_t lpf_config_of_selftest_init(void)
{
	int32_t ret;

	ret = lpf_config_of_selftest_run();
	if (ret != OSAL_SUCCESS)
		return ret;

	pr_info("LPF:CONFIG_OF_SELFTEST: checks passed\n");
	return OSAL_SUCCESS;
}

static void lpf_config_of_selftest_exit(void)
{
	pr_info("LPF:CONFIG_OF_SELFTEST: unloaded\n");
}

lpf_runtime_selftest_register(config_of_selftest, lpf_config_of_selftest_init,
			      lpf_config_of_selftest_exit);
