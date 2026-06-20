// SPDX-License-Identifier: MIT

#include "lpf/config/lpf_config.h"
#include "lpf_config_normalizer.h"

#include <string.h>

#define TEST_DEVICE_CAPACITY 8U

extern const lpf_config_platform_config_t
	g_lpf_config_kernel_x86_mock_modules_1_0_0;

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

static int expect_string_equal(const char *left, const char *right)
{
	if (!left || !right)
		return left == right ? 0 : -1;

	return strcmp(left, right);
}

static int compare_mcu_entries(const lpf_config_mcu_entry_t *left,
			       const lpf_config_mcu_entry_t *right)
{
	const lpf_config_mcu_config_t *a = &left->config;
	const lpf_config_mcu_config_t *b = &right->config;

	if (left->enabled != right->enabled)
		return -1;
	if (expect_string_equal(a->name, b->name))
		return -1;
	if (a->interface != b->interface)
		return -1;
	if (a->cmd_timeout_ms != b->cmd_timeout_ms ||
	    a->retry_count != b->retry_count)
		return -1;

	switch (a->interface) {
	case LPF_CONFIG_MCU_INTERFACE_CAN:
		if (expect_string_equal(a->hw.can.device, b->hw.can.device))
			return -1;
		return a->hw.can.bitrate == b->hw.can.bitrate &&
			       a->hw.can.rx_timeout == b->hw.can.rx_timeout &&
			       a->hw.can.tx_timeout == b->hw.can.tx_timeout &&
			       a->hw.can.tx_id == b->hw.can.tx_id &&
			       a->hw.can.rx_id == b->hw.can.rx_id ?
			       0 :
			       -1;
	case LPF_CONFIG_MCU_INTERFACE_SERIAL:
		if (expect_string_equal(a->hw.serial.device,
					b->hw.serial.device))
			return -1;
		return a->hw.serial.baudrate == b->hw.serial.baudrate &&
			       a->hw.serial.data_bits == b->hw.serial.data_bits &&
			       a->hw.serial.stop_bits == b->hw.serial.stop_bits &&
			       a->hw.serial.parity == b->hw.serial.parity &&
			       a->hw.serial.flow_control ==
				       b->hw.serial.flow_control ?
			       0 :
			       -1;
	default:
		return -1;
	}
}

static int compare_led_entries(const lpf_config_led_entry_t *left,
			       const lpf_config_led_entry_t *right)
{
	const lpf_config_led_config_t *a = &left->config;
	const lpf_config_led_config_t *b = &right->config;

	if (left->enabled != right->enabled)
		return -1;
	if (expect_string_equal(a->name, b->name))
		return -1;
	if (a->control != b->control ||
	    a->max_brightness != b->max_brightness ||
	    a->default_brightness != b->default_brightness)
		return -1;

	switch (a->control) {
	case LPF_CONFIG_LED_CONTROL_GPIO:
		return a->hw.gpio.gpio_num == b->hw.gpio.gpio_num &&
			       a->hw.gpio.pin_mux == b->hw.gpio.pin_mux &&
			       a->hw.gpio.active_low == b->hw.gpio.active_low &&
			       a->hw.gpio.pull_up == b->hw.gpio.pull_up &&
			       a->hw.gpio.pull_down == b->hw.gpio.pull_down ?
			       0 :
			       -1;
	case LPF_CONFIG_LED_CONTROL_PWM:
		if (expect_string_equal(a->hw.pwm.consumer,
					b->hw.pwm.consumer))
			return -1;
		return a->hw.pwm.period_ns == b->hw.pwm.period_ns &&
			       a->hw.pwm.polarity_inversed ==
				       b->hw.pwm.polarity_inversed ?
			       0 :
			       -1;
	default:
		return -1;
	}
}

static int compare_devices(const lpf_config_device_config_t *left,
			   const lpf_config_device_config_t *right)
{
	if (left->device_type != right->device_type || left->index != right->index)
		return -1;

	switch (left->device_type) {
	case LPF_CONFIG_DEVICE_TYPE_MCU:
		return compare_mcu_entries(left->entry, right->entry);
	case LPF_CONFIG_DEVICE_TYPE_LED:
		return compare_led_entries(left->entry, right->entry);
	default:
		return -1;
	}
}

static int normalize_platform(const lpf_config_platform_config_t *platform,
			      lpf_config_device_config_t *devices,
			      uint32_t *count)
{
	*count = TEST_DEVICE_CAPACITY;
	return lpf_config_normalize_devices(platform, devices, count);
}

static int test_static_and_dt_equivalent_normalize_same(void)
{
	lpf_config_device_config_t static_devices[TEST_DEVICE_CAPACITY];
	lpf_config_device_config_t dt_devices[TEST_DEVICE_CAPACITY];
	uint32_t static_count;
	uint32_t dt_count;
	uint32_t i;

	if (normalize_platform(&g_lpf_config_kernel_x86_mock_modules_1_0_0,
			       static_devices, &static_count) != OSAL_SUCCESS)
		return 1;
	if (normalize_platform(&g_dt_equivalent_platform, dt_devices,
			       &dt_count) != OSAL_SUCCESS)
		return 2;

	if (static_count != dt_count || static_count != 3U)
		return 3;

	for (i = 0; i < static_count; i++) {
		if (compare_devices(&static_devices[i], &dt_devices[i]))
			return 10 + (int)i;
	}

	if (static_devices[static_count].device_type !=
	    LPF_CONFIG_DEVICE_TYPE_INVALID)
		return 20;
	if (dt_devices[dt_count].device_type != LPF_CONFIG_DEVICE_TYPE_INVALID)
		return 21;

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
	lpf_config_device_config_t devices[TEST_DEVICE_CAPACITY];
	uint32_t count;

	if (normalize_platform(&platform, devices, &count) != OSAL_SUCCESS)
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

int main(void)
{
	int ret;

	ret = test_static_and_dt_equivalent_normalize_same();
	if (ret)
		return ret;

	return test_disabled_entries_keep_source_index();
}
