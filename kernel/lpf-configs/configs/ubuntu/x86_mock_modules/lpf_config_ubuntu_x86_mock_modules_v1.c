// SPDX-License-Identifier: GPL-2.0

#include "lpf_config_static.h"

static const lpf_config_mcu_entry_t
	g_lpf_config_ubuntu_x86_mock_modules_mcu0 = {
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

static const lpf_config_led_entry_t
	g_lpf_config_ubuntu_x86_mock_modules_status_led = {
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
	};

static const lpf_config_led_entry_t
	g_lpf_config_ubuntu_x86_mock_modules_activity_led = {
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
	};

static const lpf_config_device_node_t
	g_lpf_config_ubuntu_x86_mock_modules_nodes[] = {
		{
			.device_type = LPF_CONFIG_DEVICE_TYPE_MCU,
			.index = 0,
			.name = "mcu0",
			.compatible = "lpf,mcu",
			.status = LPF_CONFIG_NODE_STATUS_OKAY,
			.payload = &g_lpf_config_ubuntu_x86_mock_modules_mcu0,
			.entry = &g_lpf_config_ubuntu_x86_mock_modules_mcu0,
			.payload_size = sizeof(lpf_config_mcu_entry_t),
		},
		{
			.device_type = LPF_CONFIG_DEVICE_TYPE_LED,
			.index = 0,
			.name = "status",
			.compatible = "lpf,led",
			.status = LPF_CONFIG_NODE_STATUS_OKAY,
			.payload =
				&g_lpf_config_ubuntu_x86_mock_modules_status_led,
			.entry =
				&g_lpf_config_ubuntu_x86_mock_modules_status_led,
			.payload_size = sizeof(lpf_config_led_entry_t),
		},
		{
			.device_type = LPF_CONFIG_DEVICE_TYPE_LED,
			.index = 1,
			.name = "activity",
			.compatible = "lpf,led",
			.status = LPF_CONFIG_NODE_STATUS_OKAY,
			.payload =
				&g_lpf_config_ubuntu_x86_mock_modules_activity_led,
			.entry =
				&g_lpf_config_ubuntu_x86_mock_modules_activity_led,
			.payload_size = sizeof(lpf_config_led_entry_t),
		},
	};

static const lpf_config_platform_config_t
	g_lpf_config_ubuntu_x86_mock_modules_v1 = {
		.platform_name = "linux",
		.chip_name = "x86_64",
		.project_name = "x86_mock_modules",
		.product_name = "ubuntu",
		.version = "1.0.0",
		.device_node_count = OSAL_ARRAY_SIZE(
			g_lpf_config_ubuntu_x86_mock_modules_nodes),
		.device_nodes = g_lpf_config_ubuntu_x86_mock_modules_nodes,
		.mcu_count = 0,
		.mcu_array = NULL,
		.led_count = 0,
		.led_array = NULL,
};

lpf_config_static_register(ubuntu_x86_mock_modules_v1,
			   &g_lpf_config_ubuntu_x86_mock_modules_v1);
