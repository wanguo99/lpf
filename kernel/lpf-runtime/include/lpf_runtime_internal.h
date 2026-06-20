// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_RUNTIME_INTERNAL_H
#define LPF_RUNTIME_INTERNAL_H

#include "osal.h"
#include "lpf/config/lpf_config.h"
#include "lpf/core/lpf_device.h"

#define LPF_RUNTIME_ENTRY_SECTION ".lpf_runtime_entry"
#define LPF_RUNTIME_CONFIG_MAPPER_SECTION ".lpf_runtime_config_mapper"

typedef enum {
	LPF_RUNTIME_ENTRY_CLASS_CORE = 0,
	LPF_RUNTIME_ENTRY_CLASS_SERVICE,
	LPF_RUNTIME_ENTRY_CLASS_SELFTEST,
	LPF_RUNTIME_ENTRY_CLASS_COUNT,
} lpf_runtime_entry_class_t;

typedef struct {
	const char *name;
	lpf_runtime_entry_class_t class;
	int32_t (*init)(void);
	void (*exit)(void);
} lpf_runtime_entry_t;

typedef struct {
	lpf_config_device_type_t type;
	int32_t (*map)(const lpf_config_device_config_t *device,
		       lpf_device_config_t *config);
} lpf_runtime_config_mapper_t;

#define lpf_runtime_entry_register(_class, _id, _init, _exit)         \
	static const lpf_runtime_entry_t __lpf_runtime_entry_##_id \
		__attribute__((used, aligned(sizeof(void *)),             \
			       section(LPF_RUNTIME_ENTRY_SECTION))) = { \
			.name = #_id,                                    \
			.class = _class,                                 \
			.init = _init,                                   \
			.exit = _exit,                                   \
		}

#define lpf_runtime_core_register(_id, _init, _exit) \
	lpf_runtime_entry_register(LPF_RUNTIME_ENTRY_CLASS_CORE, _id, _init, _exit)

#define lpf_runtime_service_register(_id, _init, _exit) \
	lpf_runtime_entry_register(LPF_RUNTIME_ENTRY_CLASS_SERVICE, _id, _init, _exit)

#define lpf_runtime_selftest_register(_id, _init, _exit) \
	lpf_runtime_entry_register(LPF_RUNTIME_ENTRY_CLASS_SELFTEST, _id, _init, _exit)

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
