// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_INTERNAL_H
#define PDM_INTERNAL_H

#include "osal.h"
#include "pconfig/pconfig_common.h"

#define PDM_DRIVER_SECTION ".pdm_driver"
#define PDM_CONCAT(a, b) a##b
#define PDM_UNIQUE_ID(a, b) PDM_CONCAT(a, b)

typedef struct pdm_driver {
	const char *name;
	pconfig_device_type_t type;
	int (*init)(void);
	void (*exit)(void);
	int32_t (*probe)(const pconfig_device_config_t *device);
	void (*remove_all)(void);
} pdm_driver_t;

#define pdm_driver_register(_driver)                                      \
	static const pdm_driver_t *const PDM_UNIQUE_ID(__pdm_driver_,     \
						       __LINE__)          \
		__attribute__((used, aligned(sizeof(void *)),              \
			       section(PDM_DRIVER_SECTION))) = (_driver)

extern const pdm_driver_t *const pdm_driver_start;
extern const pdm_driver_t *const pdm_driver_end;

#endif /* PDM_INTERNAL_H */
