// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_CORE_H
#define LPF_CORE_H

#include "lpf/lpf_device.h"
#include "lpf/lpf_driver.h"

int32_t lpf_core_init(void);
void lpf_core_deinit(void);

int32_t lpf_driver_register(const lpf_driver_t *driver);
void lpf_driver_unregister(const lpf_driver_t *driver);
void lpf_driver_unregister_all(void);

int32_t lpf_device_register(const lpf_device_config_t *config);
void lpf_device_unregister_all(void);
const lpf_device_t *lpf_device_find(lpf_device_type_t type, uint32_t index);
int32_t lpf_device_get_info(lpf_device_type_t type, uint32_t index,
			    lpf_device_info_t *info);
int32_t lpf_device_get_info_by_name(const char *name,
				    lpf_device_info_t *info);
int32_t lpf_device_get_info_by_capability(lpf_capability_t required,
					  uint32_t match_index,
					  lpf_device_info_t *info);
int32_t lpf_device_list(lpf_device_info_t *infos, uint32_t *count);
void lpf_device_record_error(lpf_device_type_t type, uint32_t index,
			     int32_t error);

#endif /* LPF_CORE_H */
