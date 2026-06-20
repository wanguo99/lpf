// SPDX-License-Identifier: GPL-2.0

#include "lpf_runtime_internal.h"

const lpf_runtime_entry_t lpf_runtime_entry_start
	__attribute__((used, aligned(sizeof(void *)),
		       section(LPF_RUNTIME_ENTRY_SECTION))) = {};

const lpf_runtime_config_mapper_t lpf_runtime_config_mapper_start
	__attribute__((used, aligned(sizeof(void *)),
		       section(LPF_RUNTIME_CONFIG_MAPPER_SECTION))) = {};
