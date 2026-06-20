/************************************************************************
 * LPF_CONFIG LED configuration types
 ************************************************************************/

#ifndef LPF_CONFIG_LED_H
#define LPF_CONFIG_LED_H

#include "lpf/config/lpf_config_common.h"

typedef enum {
	LPF_CONFIG_LED_CONTROL_GPIO = 0x00,
	LPF_CONFIG_LED_CONTROL_PWM = 0x01,
} lpf_config_led_control_t;

typedef struct {
	const char *consumer;
	uint32_t period_ns;
	bool polarity_inversed;
} lpf_config_pwm_config_t;

typedef struct {
	char name[64];
	lpf_config_led_control_t control;
	uint32_t max_brightness;
	uint32_t default_brightness;
	union {
		lpf_config_gpio_config_t gpio;
		lpf_config_pwm_config_t pwm;
	} hw;
} lpf_config_led_config_t;

typedef struct {
	const char *description;
	bool enabled;
	lpf_config_led_config_t config;
} lpf_config_led_entry_t;

#endif /* LPF_CONFIG_LED_H */
