// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_PWM_H
#define LPF_HW_PWM_H

#include "lpf/lpf_hw_types.h"

int32_t lpf_hw_pwm_init(const lpf_pwm_config_t *config,
			lpf_hw_pwm_handle_t *handle);
int32_t lpf_hw_pwm_deinit(lpf_hw_pwm_handle_t handle);
int32_t lpf_hw_pwm_apply(lpf_hw_pwm_handle_t handle,
			 const lpf_pwm_state_t *state);
int32_t lpf_hw_pwm_get_state(lpf_hw_pwm_handle_t handle,
			     lpf_pwm_state_t *state);
int32_t lpf_hw_pwm_enable(lpf_hw_pwm_handle_t handle);
int32_t lpf_hw_pwm_disable(lpf_hw_pwm_handle_t handle);

#endif /* LPF_HW_PWM_H */
