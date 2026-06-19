// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_LED_INTERNAL_H
#define PDM_LED_INTERNAL_H

#include "pconfig.h"
#include "pdm_led.h"

#ifndef CONFIG_PDM_LED_MAX_DEVICES
#define CONFIG_PDM_LED_MAX_DEVICES 8
#endif

#define PDM_LED_MAX_DEVICES CONFIG_PDM_LED_MAX_DEVICES

int32_t pdm_led_probe(const pconfig_device_config_t *device);
void pdm_led_remove_all(void);
pdm_led_handle_t pdm_led_get(uint32_t index);

int pdm_led_chrdev_register(void);
void pdm_led_chrdev_unregister(void);

#endif /* PDM_LED_INTERNAL_H */
