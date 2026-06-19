// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_COMPAT_PWM_H
#define LPF_COMPAT_PWM_H

#include "lpf/lpf_hw_types.h"

int32_t lpf_compat_pwm_get(const char *consumer, lpf_pwm_handle_t *handle);
void lpf_compat_pwm_put(lpf_pwm_handle_t handle);
int32_t lpf_compat_pwm_apply(lpf_pwm_handle_t handle,
			     const lpf_pwm_state_t *state);
int32_t lpf_compat_pwm_get_state(lpf_pwm_handle_t handle,
				 lpf_pwm_state_t *state);
int32_t lpf_compat_pwm_enable(lpf_pwm_handle_t handle);
int32_t lpf_compat_pwm_disable(lpf_pwm_handle_t handle);

#endif /* LPF_COMPAT_PWM_H */
