// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_backend.h
 * @brief Section-based PDM peripheral backend registration
 */

#ifndef PDM_BACKEND_H
#define PDM_BACKEND_H

#include <linux/compiler.h>
#include <linux/mod_devicetable.h>
#include <linux/types.h>

#include "pdm/pdm_ctl.h"

enum pdm_backend_class {
	PDM_BACKEND_CLASS_TRANSPORT = 0,
	PDM_BACKEND_CLASS_CONTROL,
	PDM_BACKEND_CLASS_HW,
};

struct pdm_backend_entry {
	const char *name;
	u32 device_type;
	u32 backend_class;
	const struct of_device_id *matches;
	const void *ops;
	int (*init)(void);
	void (*exit)(void);
};

#define pdm_backend_register(_name, _device_type, _class, _matches, _ops, \
			     _init, _exit) \
	static const struct pdm_backend_entry \
	__pdm_backend_entry_##_name __used \
	__section("pdm_backend_entries") __aligned(sizeof(void *)) = { \
		.name = #_name, \
		.device_type = _device_type, \
		.backend_class = _class, \
		.matches = _matches, \
		.ops = _ops, \
		.init = _init, \
		.exit = _exit, \
	}

int pdm_backend_entries_init(void);
void pdm_backend_entries_exit(void);
const struct pdm_backend_entry *pdm_backend_find(u32 device_type,
						 u32 backend_class,
						 const char *compatible);

#endif /* PDM_BACKEND_H */
