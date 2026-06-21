// SPDX-License-Identifier: GPL-2.0

#include "lpf_config_static.h"

static const lpf_config_gpio_config_t
	g_lpf_config_imx6ull_modules_mcu0_reset_gpio = {
		.gpio_num = 0U,
		.pin_mux = 0U,
		.active_low = true,
		.pull_up = false,
		.pull_down = false,
	};

static const lpf_config_gpio_config_t
	g_lpf_config_imx6ull_modules_mcu0_irq_gpio = {
		.gpio_num = 0U,
		.pin_mux = 0U,
		.active_low = false,
		.pull_up = false,
		.pull_down = false,
	};

static const lpf_config_mcu_entry_t
	g_lpf_config_imx6ull_modules_mcus[] = {
		{
			.description = "i.MX6ULL placeholder CAN MCU",
			.enabled = false,
			.config = {
				.name = "mcu0",
				.interface = LPF_CONFIG_MCU_INTERFACE_CAN,
				.hw.can = {
					.device = "can0",
					.bitrate = 500000U,
					.rx_timeout = 10U,
					.tx_timeout = 10U,
					.tx_id = 0x123U,
					.rx_id = 0x123U,
				},
				.cmd_timeout_ms = 1000U,
				.retry_count = 0U,
			},
			.reset_gpio =
				&g_lpf_config_imx6ull_modules_mcu0_reset_gpio,
			.irq_gpio =
				&g_lpf_config_imx6ull_modules_mcu0_irq_gpio,
		},
	};

static const lpf_config_led_entry_t
	g_lpf_config_imx6ull_modules_leds[] = {
		{
			.description = "i.MX6ULL placeholder GPIO status LED",
			.enabled = false,
			.config = {
				.name = "status",
				.control = LPF_CONFIG_LED_CONTROL_GPIO,
				.max_brightness = 1U,
				.default_brightness = 0U,
				.hw.gpio = {
					.gpio_num = 0U,
					.pin_mux = 0U,
					.active_low = false,
					.pull_up = false,
					.pull_down = false,
				},
			},
		},
		{
			.description = "i.MX6ULL placeholder PWM activity LED",
			.enabled = false,
			.config = {
				.name = "activity",
				.control = LPF_CONFIG_LED_CONTROL_PWM,
				.max_brightness = 255U,
				.default_brightness = 0U,
				.hw.pwm = {
					.consumer = "lpf-activity",
					.period_ns = 1000000U,
					.polarity_inversed = false,
				},
			},
		},
	};

static const lpf_config_device_node_t
	g_lpf_config_imx6ull_modules_nodes[] = {
		{
			.device_type = LPF_CONFIG_DEVICE_TYPE_MCU,
			.index = 0,
			.name = "mcu0",
			.compatible = "lpf,mcu",
			.status = LPF_CONFIG_NODE_STATUS_DISABLED,
			.payload = &g_lpf_config_imx6ull_modules_mcus[0],
			.entry = &g_lpf_config_imx6ull_modules_mcus[0],
			.payload_size = sizeof(lpf_config_mcu_entry_t),
		},
		{
			.device_type = LPF_CONFIG_DEVICE_TYPE_LED,
			.index = 0,
			.name = "status",
			.compatible = "lpf,led",
			.status = LPF_CONFIG_NODE_STATUS_DISABLED,
			.payload = &g_lpf_config_imx6ull_modules_leds[0],
			.entry = &g_lpf_config_imx6ull_modules_leds[0],
			.payload_size = sizeof(lpf_config_led_entry_t),
		},
		{
			.device_type = LPF_CONFIG_DEVICE_TYPE_LED,
			.index = 1,
			.name = "activity",
			.compatible = "lpf,led",
			.status = LPF_CONFIG_NODE_STATUS_DISABLED,
			.payload = &g_lpf_config_imx6ull_modules_leds[1],
			.entry = &g_lpf_config_imx6ull_modules_leds[1],
			.payload_size = sizeof(lpf_config_led_entry_t),
		},
	};

static const lpf_config_platform_config_t
	g_lpf_config_imx6ull_modules_v1 = {
		.platform_name = "nxp",
		.chip_name = "imx6ull",
		.project_name = "modules",
		.product_name = "imx6ull",
		.version = "1.0.0",
		.device_node_count = OSAL_ARRAY_SIZE(
			g_lpf_config_imx6ull_modules_nodes),
		.device_nodes = g_lpf_config_imx6ull_modules_nodes,
		.mcu_count = OSAL_ARRAY_SIZE(g_lpf_config_imx6ull_modules_mcus),
		.mcu_array = g_lpf_config_imx6ull_modules_mcus,
		.led_count = OSAL_ARRAY_SIZE(g_lpf_config_imx6ull_modules_leds),
		.led_array = g_lpf_config_imx6ull_modules_leds,
	};

lpf_config_static_register(imx6ull_modules_v1,
			   &g_lpf_config_imx6ull_modules_v1);
