// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_LED_INTERNAL_H
#define LPF_LED_INTERNAL_H

#include "osal.h"
#include "lpf/config/lpf_config.h"
#include "lpf/core/lpf_core.h"
#include "lpf/peripheral/led/lpf_led_service.h"

#ifndef CONFIG_LPF_LED_MAX_DEVICES
#define CONFIG_LPF_LED_MAX_DEVICES 8
#endif

#define LPF_LED_MAX_DEVICES CONFIG_LPF_LED_MAX_DEVICES

typedef struct lpf_led_context {
	const lpf_config_led_config_t *config;
	void *hw_handle;
	osal_mutex_t lock;
	uint32_t brightness;
	bool enabled;
	bool lock_ready;
	uint32_t index;
	struct lpf_led_context *next;
} lpf_led_context_t;

typedef struct {
	lpf_config_led_control_t control;
	int32_t (*init)(lpf_led_context_t *ctx);
	int32_t (*apply)(lpf_led_context_t *ctx);
	void (*deinit)(lpf_led_context_t *ctx);
} lpf_led_control_ops_t;

typedef struct {
	bool present;
	char name[64];
	lpf_config_led_control_t control;
	bool enabled;
	uint32_t brightness;
	uint32_t max_brightness;
} lpf_led_debug_info_t;

int32_t lpf_led_probe(const lpf_device_t *device);
void lpf_led_remove(const lpf_device_t *device);
lpf_led_handle_t lpf_led_get(uint32_t index);
int32_t lpf_led_debug_get(uint32_t index, lpf_led_debug_info_t *info);
const lpf_led_control_ops_t *
lpf_led_control_get(lpf_config_led_control_t control);
extern const lpf_led_control_ops_t lpf_led_gpio_ops;
extern const lpf_led_control_ops_t lpf_led_pwm_ops;

int lpf_led_chrdev_register(void);
void lpf_led_chrdev_unregister(void);
int lpf_led_chrdev_register_device(const lpf_device_t *device);
void lpf_led_chrdev_unregister_device(const lpf_device_t *device);
void lpf_led_chrdev_record_error(uint32_t index, int error);
void lpf_led_chrdev_record_recovery(uint32_t index);
int lpf_led_proc_register(void);
void lpf_led_proc_unregister(void);
int lpf_led_debugfs_register(void);
void lpf_led_debugfs_unregister(void);

#endif /* LPF_LED_INTERNAL_H */
