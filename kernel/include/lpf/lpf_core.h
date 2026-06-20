// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_CORE_H
#define LPF_CORE_H

#include "lpf/lpf_device.h"
#include "lpf/lpf_driver.h"

typedef struct lpf_device_handle lpf_device_handle_t;

typedef enum {
	LPF_DEVICE_EVENT_REGISTERED = 0,
	LPF_DEVICE_EVENT_BOUND,
	LPF_DEVICE_EVENT_STATE_CHANGED,
	LPF_DEVICE_EVENT_ERROR,
	LPF_DEVICE_EVENT_REMOVING,
	LPF_DEVICE_EVENT_REMOVED,
} lpf_device_event_type_t;

typedef struct {
	lpf_device_event_type_t type;
	lpf_device_info_t device;
	int32_t status;
} lpf_device_event_t;

typedef void (*lpf_device_event_callback_t)(
	const lpf_device_event_t *event, void *user_data);

int32_t lpf_core_init(void);
void lpf_core_deinit(void);

int32_t lpf_driver_register(const lpf_driver_t *driver);
void lpf_driver_unregister(const lpf_driver_t *driver);
void lpf_driver_unregister_all(void);

int32_t lpf_device_register(const lpf_device_config_t *config);
void lpf_device_unregister_all(void);
const lpf_device_t *lpf_device_find(lpf_device_type_t type, uint32_t index);
lpf_device_handle_t *lpf_device_get(lpf_device_type_t type, uint32_t index);
lpf_device_handle_t *lpf_device_get_by_name(const char *name);
lpf_device_handle_t *
lpf_device_get_by_capability(lpf_capability_t required, uint32_t match_index);
const lpf_device_t *lpf_device_from_handle(
	const lpf_device_handle_t *handle);
int32_t lpf_device_handle_get_info(const lpf_device_handle_t *handle,
				   lpf_device_info_t *info);
void lpf_device_put(lpf_device_handle_t *handle);
int32_t lpf_device_get_info(lpf_device_type_t type, uint32_t index,
			    lpf_device_info_t *info);
int32_t lpf_device_get_info_by_name(const char *name,
				    lpf_device_info_t *info);
int32_t lpf_device_get_info_by_capability(lpf_capability_t required,
					  uint32_t match_index,
					  lpf_device_info_t *info);
int32_t lpf_device_list(lpf_device_info_t *infos, uint32_t *count);
int32_t lpf_device_set_state(lpf_device_type_t type, uint32_t index,
			     lpf_device_state_t state, int32_t status);
void lpf_device_record_error(lpf_device_type_t type, uint32_t index,
			     int32_t error);
int32_t lpf_device_event_subscribe(lpf_device_event_callback_t callback,
				   void *user_data);
void lpf_device_event_unsubscribe(lpf_device_event_callback_t callback,
				  void *user_data);

#endif /* LPF_CORE_H */
