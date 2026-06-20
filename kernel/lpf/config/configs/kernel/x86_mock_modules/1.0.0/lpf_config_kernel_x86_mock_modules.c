// SPDX-License-Identifier: GPL-2.0

#include "lpf/lpf_config.h"

static const lpf_config_mcu_entry_t
	g_lpf_config_kernel_x86_mock_modules_mcus[] = {
		{
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
		},
	};

static const lpf_config_led_entry_t
	g_lpf_config_kernel_x86_mock_modules_leds[] = {
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

const lpf_config_platform_config_t
	g_lpf_config_kernel_x86_mock_modules_1_0_0 = {
		.platform_name = "linux",
		.chip_name = "x86_64",
		.project_name = "x86_mock_modules",
		.product_name = "kernel",
		.version = "1.0.0",
		.mcu_count = OSAL_ARRAY_SIZE(
			g_lpf_config_kernel_x86_mock_modules_mcus),
		.mcu_array = g_lpf_config_kernel_x86_mock_modules_mcus,
		.led_count = OSAL_ARRAY_SIZE(
			g_lpf_config_kernel_x86_mock_modules_leds),
		.led_array = g_lpf_config_kernel_x86_mock_modules_leds,
	};
