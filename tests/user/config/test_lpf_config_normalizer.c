// SPDX-License-Identifier: MIT

#include "lpf/config/lpf_config.h"
#include "lpf_config_normalizer.h"
#include "test_lpf_config_compare.h"

extern const lpf_config_platform_config_t
	g_lpf_config_kernel_x86_mock_modules_v1;

static const lpf_config_mcu_entry_t g_dt_equivalent_mcus[] = {
	{
		.description = "mcu0",
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
	},
};

static const lpf_config_led_entry_t g_dt_equivalent_leds[] = {
	{
		.description = "status",
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
		.description = "activity",
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

static const lpf_config_platform_config_t g_dt_equivalent_platform = {
	.platform_name = "linux",
	.chip_name = "x86_64",
	.project_name = "x86_mock_modules",
	.product_name = "kernel",
	.version = "1.0.0",
	.mcu_count = OSAL_ARRAY_SIZE(g_dt_equivalent_mcus),
	.mcu_array = g_dt_equivalent_mcus,
	.led_count = OSAL_ARRAY_SIZE(g_dt_equivalent_leds),
	.led_array = g_dt_equivalent_leds,
};

static int test_static_and_dt_equivalent_normalize_same(void)
{
	lpf_config_device_config_t
		static_devices[TEST_LPF_CONFIG_DEVICE_CAPACITY];
	lpf_config_device_config_t dt_devices[TEST_LPF_CONFIG_DEVICE_CAPACITY];
	uint32_t static_count;
	uint32_t dt_count;
	uint32_t i;

	if (test_lpf_config_normalize_platform(
		    &g_lpf_config_kernel_x86_mock_modules_v1, static_devices,
		    &static_count) != OSAL_SUCCESS)
		return 1;
	if (test_lpf_config_normalize_platform(&g_dt_equivalent_platform,
					       dt_devices,
					       &dt_count) != OSAL_SUCCESS)
		return 2;

	if (static_count != dt_count || static_count != 3U)
		return 3;

	for (i = 0; i < static_count; i++) {
		if (test_lpf_config_compare_devices(&static_devices[i],
						    &dt_devices[i]))
			return 10 + (int)i;
	}

	if (static_devices[static_count].device_type !=
	    LPF_CONFIG_DEVICE_TYPE_INVALID)
		return 20;
	if (dt_devices[dt_count].device_type != LPF_CONFIG_DEVICE_TYPE_INVALID)
		return 21;
	if (static_devices[0].status != LPF_CONFIG_NODE_STATUS_OKAY ||
	    test_lpf_config_string_equal(static_devices[0].name, "mcu0") ||
	    test_lpf_config_string_equal(static_devices[0].compatible,
					 "lpf,mcu") ||
	    static_devices[0].payload != static_devices[0].entry ||
	    static_devices[0].payload_size != sizeof(lpf_config_mcu_entry_t))
		return 22;
	if (static_devices[1].status != LPF_CONFIG_NODE_STATUS_OKAY ||
	    test_lpf_config_string_equal(static_devices[1].name, "status") ||
	    test_lpf_config_string_equal(static_devices[1].compatible,
					 "lpf,led") ||
	    static_devices[1].payload != static_devices[1].entry ||
	    static_devices[1].payload_size != sizeof(lpf_config_led_entry_t))
		return 23;

	return 0;
}

static int test_disabled_entries_keep_source_index(void)
{
	static const lpf_config_mcu_entry_t mcu_entries[] = {
		{
			.description = "disabled",
			.enabled = false,
			.config = {
				.name = "disabled",
				.interface = LPF_CONFIG_MCU_INTERFACE_CAN,
				.hw.can = {
					.device = "mock-can-disabled",
					.bitrate = 500000U,
					.rx_timeout = 10U,
					.tx_timeout = 10U,
					.tx_id = 0x10U,
					.rx_id = 0x10U,
				},
				.cmd_timeout_ms = 1000U,
				.retry_count = 0U,
			},
		},
		{
			.description = "enabled",
			.enabled = true,
			.config = {
				.name = "enabled",
				.interface = LPF_CONFIG_MCU_INTERFACE_CAN,
				.hw.can = {
					.device = "mock-can-enabled",
					.bitrate = 500000U,
					.rx_timeout = 10U,
					.tx_timeout = 10U,
					.tx_id = 0x11U,
					.rx_id = 0x11U,
				},
				.cmd_timeout_ms = 1000U,
				.retry_count = 0U,
			},
		},
	};
	static const lpf_config_platform_config_t platform = {
		.platform_name = "linux",
		.chip_name = "x86_64",
		.project_name = "test",
		.product_name = "kernel",
		.version = "1.0.0",
		.mcu_count = OSAL_ARRAY_SIZE(mcu_entries),
		.mcu_array = mcu_entries,
		.led_count = 0,
		.led_array = NULL,
	};
	lpf_config_device_config_t devices[TEST_LPF_CONFIG_DEVICE_CAPACITY];
	uint32_t count;

	if (test_lpf_config_normalize_platform(&platform, devices,
					       &count) != OSAL_SUCCESS)
		return 101;
	if (count != 1U)
		return 102;
	if (devices[0].device_type != LPF_CONFIG_DEVICE_TYPE_MCU ||
	    devices[0].index != 1U || devices[0].entry != &mcu_entries[1])
		return 103;
	if (devices[1].device_type != LPF_CONFIG_DEVICE_TYPE_INVALID)
		return 104;

	return 0;
}

static int test_count_only_reports_enabled_devices(void)
{
	uint32_t count = 0;

	if (lpf_config_normalize_devices(
		    &g_lpf_config_kernel_x86_mock_modules_v1, NULL,
		    &count) != OSAL_SUCCESS)
		return 201;

	return count == 3U ? 0 : 202;
}

static int test_node_table_supports_compat_accessors(void)
{
	const lpf_config_mcu_entry_t *mcu;
	const lpf_config_led_entry_t *status;
	const lpf_config_led_entry_t *activity;

	mcu = lpf_config_hw_get_mcu(&g_lpf_config_kernel_x86_mock_modules_v1,
				    0);
	status = lpf_config_hw_get_led(
		&g_lpf_config_kernel_x86_mock_modules_v1, 0);
	activity = lpf_config_hw_get_led(
		&g_lpf_config_kernel_x86_mock_modules_v1, 1);

	if (!mcu || !status || !activity)
		return 401;
	if (test_lpf_config_string_equal(mcu->config.name, "mcu0") ||
	    test_lpf_config_string_equal(status->config.name, "status") ||
	    test_lpf_config_string_equal(activity->config.name, "activity"))
		return 402;

	return 0;
}

static int test_exact_capacity_requires_sentinel_slot(void)
{
	lpf_config_device_config_t devices[3];
	uint32_t count = OSAL_ARRAY_SIZE(devices);

	if (lpf_config_normalize_devices(
		    &g_lpf_config_kernel_x86_mock_modules_v1, devices,
		    &count) != OSAL_ERR_RESOURCE_LIMIT)
		return 301;
	if (count != 3U)
		return 302;

	return 0;
}

int main(void)
{
	int ret;

	ret = test_static_and_dt_equivalent_normalize_same();
	if (ret)
		return ret;

	ret = test_disabled_entries_keep_source_index();
	if (ret)
		return ret;

	ret = test_count_only_reports_enabled_devices();
	if (ret)
		return ret;

	ret = test_node_table_supports_compat_accessors();
	if (ret)
		return ret;

	return test_exact_capacity_requires_sentinel_slot();
}
