// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_DRIVER_H
#define LPF_DRIVER_H

#include "lpf/lpf_device.h"

typedef struct lpf_driver {
	const char *name;
	lpf_device_type_t type;
	lpf_capability_t capabilities;
	int (*init)(void);
	void (*exit)(void);
	int32_t (*probe)(const lpf_device_t *device);
	void (*remove)(const lpf_device_t *device);
} lpf_driver_t;

#endif /* LPF_DRIVER_H */
