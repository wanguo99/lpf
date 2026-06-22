// SPDX-License-Identifier: GPL-2.0

#include "lpf/config/lpf_config_static.h"

static const lpf_config_platform_config_t
	g_lpf_config_ubuntu_x86_modules_v1 = {
	.platform_name = "linux",
	.chip_name = "x86_64",
	.project_name = "x86_modules",
	.product_name = "ubuntu",
	.version = "1.0.0",
	.device_node_count = 0,
	.device_nodes = NULL,
	.mcu_count = 0,
	.mcu_array = NULL,
	.led_count = 0,
	.led_array = NULL,
};

lpf_config_static_register(ubuntu_x86_modules_v1,
			   &g_lpf_config_ubuntu_x86_modules_v1);
