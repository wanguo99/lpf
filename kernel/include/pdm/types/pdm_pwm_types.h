// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_PWM_TYPES_H
#define LPF_PWM_TYPES_H

#include "osal.h"

typedef void *lpf_hw_pwm_handle_t;
typedef void *lpf_pwm_handle_t;

typedef struct {
	const char *consumer;
	uint32_t period_ns;
	uint32_t duty_ns;
	bool enabled;
	bool polarity_inversed;
} lpf_pwm_config_t;

typedef struct {
	uint32_t period_ns;
	uint32_t duty_ns;
	bool enabled;
	bool polarity_inversed;
} lpf_pwm_state_t;

#endif /* LPF_PWM_TYPES_H */
