// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_INTERNAL_H
#define PDM_INTERNAL_H

#include "osal.h"
#include "lpf/lpf_core.h"
#include "pconfig/pconfig_common.h"

typedef lpf_driver_t pdm_driver_t;
typedef lpf_device_t pdm_device_t;

int32_t pdm_register_builtin_drivers(void);
void pdm_unregister_builtin_drivers(void);

#endif /* PDM_INTERNAL_H */
