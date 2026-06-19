/************************************************************************
 * HAL PWM hardware abstraction API
 ************************************************************************/

#ifndef HAL_PWM_H
#define HAL_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void *hal_pwm_handle_t;

typedef struct {
	const char *consumer;
	uint32_t period_ns;
	uint32_t duty_ns;
	bool enabled;
	bool polarity_inversed;
} hal_pwm_config_t;

typedef struct {
	uint32_t period_ns;
	uint32_t duty_ns;
	bool enabled;
	bool polarity_inversed;
} hal_pwm_state_t;

int32_t hal_pwm_init(const hal_pwm_config_t *config, hal_pwm_handle_t *handle);
int32_t hal_pwm_deinit(hal_pwm_handle_t handle);
int32_t hal_pwm_apply(hal_pwm_handle_t handle, const hal_pwm_state_t *state);
int32_t hal_pwm_get_state(hal_pwm_handle_t handle, hal_pwm_state_t *state);
int32_t hal_pwm_enable(hal_pwm_handle_t handle);
int32_t hal_pwm_disable(hal_pwm_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* HAL_PWM_H */
