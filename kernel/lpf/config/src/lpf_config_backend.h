/* SPDX-License-Identifier: GPL-2.0 */

#ifndef LPF_CONFIG_BACKEND_H
#define LPF_CONFIG_BACKEND_H

#include "lpf/config/lpf_config.h"

typedef struct {
	const char *name;
	bool (*available)(void);
	int32_t (*load)(void);
	void (*unload)(void);
	const lpf_config_platform_config_t *(*active)(void);
	const lpf_config_platform_config_t *(*find)(const char *product,
						  const char *project,
						  const char *version);
	int32_t (*list)(const lpf_config_platform_config_t **configs,
			uint32_t *count);
} lpf_config_backend_ops_t;

const lpf_config_backend_ops_t *lpf_config_backend_select(void);

extern const lpf_config_backend_ops_t g_lpf_config_dt_backend;
extern const lpf_config_backend_ops_t g_lpf_config_static_backend;

#endif /* LPF_CONFIG_BACKEND_H */
