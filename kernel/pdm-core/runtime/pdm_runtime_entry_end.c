// SPDX-License-Identifier: GPL-2.0

#include "pdm_runtime_internal.h"

const pdm_runtime_entry_t pdm_runtime_entry_end
	__attribute__((used, aligned(sizeof(void *)),
		       section(PDM_RUNTIME_ENTRY_SECTION))) = {};

/* 已禁用：CONFIG_DRIVER 系统依赖 pdm_configs
const pdm_runtime_config_driver_t pdm_runtime_config_driver_end
	__attribute__((used, aligned(sizeof(void *)),
		       section(PDM_RUNTIME_CONFIG_DRIVER_SECTION))) = {};
*/
