// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_INTERNAL_H
#define PDM_INTERNAL_H

#include "osal.h"
#include "pconfig/pconfig_common.h"

#define PDM_BUILTIN_DRIVER_SECTION ".pdm_builtin_driver"

typedef struct {
	int32_t (*probe)(const pconfig_device_config_t *device);
	void (*remove_all)(void);
} pdm_driver_ops_t;

typedef struct {
	const char *name;
	int (*init)(void);
	void (*exit)(void);
} pdm_builtin_driver_t;

#define PDM_BUILTIN_DRIVER(_id, _init, _exit)                         \
	static const pdm_builtin_driver_t __pdm_builtin_driver_##_id   \
		__attribute__((used, aligned(sizeof(void *)),           \
			       section(PDM_BUILTIN_DRIVER_SECTION))) = { \
			.name = #_id,                                  \
			.init = _init,                                 \
			.exit = _exit,                                 \
		}

extern const pdm_builtin_driver_t pdm_builtin_driver_start;
extern const pdm_builtin_driver_t pdm_builtin_driver_end;

int32_t pdm_driver_register(pconfig_device_type_t type,
			    const pdm_driver_ops_t *ops);
void pdm_driver_unregister(pconfig_device_type_t type,
			   const pdm_driver_ops_t *ops);

#endif /* PDM_INTERNAL_H */
