// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_CONFIG_STATIC_H
#define LPF_CONFIG_STATIC_H

#include "lpf/config/lpf_config.h"

#define LPF_CONFIG_STATIC_SECTION ".lpf_config_static"

typedef struct {
	const lpf_config_platform_config_t *const *configs;
	uint32_t count;
	uint32_t current_index;
} lpf_config_static_table_t;

#define lpf_config_static_register(_id, _config)                    \
	static const lpf_config_platform_config_t *const             \
		__lpf_config_static_##_id __attribute__((used,       \
		aligned(sizeof(void *)),                             \
		section(LPF_CONFIG_STATIC_SECTION))) = _config

extern lpf_config_static_table_t g_lpf_config_platform_table;

#endif /* LPF_CONFIG_STATIC_H */
