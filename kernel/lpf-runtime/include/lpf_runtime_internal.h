// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_RUNTIME_INTERNAL_H
#define LPF_RUNTIME_INTERNAL_H

#include "osal.h"
#include "lpf/config/lpf_config.h"
#include "lpf/core/lpf_device.h"

#define LPF_RUNTIME_ENTRY_SECTION ".lpf_runtime_entry"
#define LPF_RUNTIME_CONFIG_MAPPER_SECTION ".lpf_runtime_config_mapper"

typedef struct {
	const char *name;
	int32_t (*init)(void);
	void (*exit)(void);
} lpf_runtime_entry_t;

typedef struct {
	lpf_config_device_type_t type;
	int32_t (*map)(const lpf_config_device_config_t *device,
		       lpf_device_config_t *config);
} lpf_runtime_config_mapper_t;

#define lpf_runtime_register(_id, _init, _exit)                       \
	static const lpf_runtime_entry_t __lpf_runtime_entry_##_id \
		__attribute__((used, aligned(sizeof(void *)),             \
			       section(LPF_RUNTIME_ENTRY_SECTION))) = { \
			.name = #_id,                                    \
			.init = _init,                                   \
			.exit = _exit,                                   \
		}

#define lpf_runtime_config_mapper_register(_id, _type, _map)             \
	static const lpf_runtime_config_mapper_t                         \
		__lpf_runtime_config_mapper_##_id __attribute__((used,   \
		aligned(sizeof(void *)),                                 \
		section(LPF_RUNTIME_CONFIG_MAPPER_SECTION))) = {         \
			.type = _type,                                   \
			.map = _map,                                     \
		}

extern const lpf_runtime_entry_t lpf_runtime_entry_start;
extern const lpf_runtime_entry_t lpf_runtime_entry_end;
extern const lpf_runtime_config_mapper_t lpf_runtime_config_mapper_start;
extern const lpf_runtime_config_mapper_t lpf_runtime_config_mapper_end;
int32_t lpf_runtime_probe_devices(void);

#endif /* LPF_RUNTIME_INTERNAL_H */
