// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_LED_INTERNAL_H
#define PDM_LED_INTERNAL_H

#include <linux/mutex.h>
#include <linux/types.h>

#include "pdm/core/chardev/pdm_client.h"
#include "pdm/core/device/pdm_device.h"
#include "pdm/core/driver/pdm_driver.h"

#define PDM_LED_DEFAULT_MAX_BRIGHTNESS 255U

enum pdm_led_backend_type {
	PDM_LED_BACKEND_MEMORY = 0,
	PDM_LED_BACKEND_GPIO,
	PDM_LED_BACKEND_PWM,
};

struct gpio_desc;
struct pwm_device;
struct pdm_led_instance;

struct pdm_led_backend_ops {
	enum pdm_led_backend_type type;
	const char *name;
	u64 capability;
	int (*setup)(struct pdm_led_instance *inst);
	void (*cleanup)(struct pdm_led_instance *inst);
	int (*apply)(struct pdm_led_instance *inst);
};

struct pdm_led_instance {
	struct pdm_driver_instance base;
	const struct pdm_led_backend_ops *ops;
	u32 brightness;
	u32 max_brightness;
	u32 enabled;
	union {
		struct gpio_desc *gpiod;
		struct pwm_device *pwmdev;
	} hw;
};

#endif /* PDM_LED_INTERNAL_H */
