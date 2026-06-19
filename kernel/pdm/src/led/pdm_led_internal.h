// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_LED_INTERNAL_H
#define PDM_LED_INTERNAL_H

#include "pconfig.h"
#include "pdm_led.h"

#ifndef CONFIG_PDM_LED_MAX_DEVICES
#define CONFIG_PDM_LED_MAX_DEVICES 8
#endif

#define PDM_LED_MAX_DEVICES CONFIG_PDM_LED_MAX_DEVICES

typedef struct {
	bool present;
	char name[64];
	pconfig_led_control_t control;
	bool enabled;
	uint32_t brightness;
	uint32_t max_brightness;
} pdm_led_debug_info_t;

int32_t pdm_led_probe(const pconfig_device_config_t *device);
void pdm_led_remove(const pconfig_device_config_t *device);
pdm_led_handle_t pdm_led_get(uint32_t index);
int32_t pdm_led_debug_get(uint32_t index, pdm_led_debug_info_t *info);

int pdm_led_chrdev_register(void);
void pdm_led_chrdev_unregister(void);
int pdm_led_proc_register(void);
void pdm_led_proc_unregister(void);

#endif /* PDM_LED_INTERNAL_H */
