// SPDX-License-Identifier: GPL-2.0

#include "lpf/config/lpf_config_static.h"

const lpf_config_platform_config_t *const lpf_config_static_start
	__attribute__((used, aligned(sizeof(void *)),
		       section(LPF_CONFIG_STATIC_SECTION))) = NULL;
