// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_LED_SERVICE_H
#define LPF_LED_SERVICE_H

#include "osal.h"

#define LPF_LED_SERVICE_VERSION_MAJOR 0x01
#define LPF_LED_SERVICE_VERSION_MINOR 0x00
#define LPF_LED_SERVICE_VERSION_PATCH 0x00

typedef void *lpf_led_handle_t;

typedef struct {
	uint32_t brightness;
	uint32_t max_brightness;
	bool enabled;
} lpf_led_service_state_t;

int32_t lpf_led_service_register(void);
void lpf_led_service_unregister(void);
lpf_led_handle_t lpf_led_get(uint32_t index);
int32_t lpf_led_set_brightness(lpf_led_handle_t handle, uint32_t brightness);
int32_t lpf_led_get_state(lpf_led_handle_t handle,
			  lpf_led_service_state_t *state);
int32_t lpf_led_enable(lpf_led_handle_t handle);
int32_t lpf_led_disable(lpf_led_handle_t handle);

#endif /* LPF_LED_SERVICE_H */
