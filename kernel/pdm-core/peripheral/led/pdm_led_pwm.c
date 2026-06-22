// SPDX-License-Identifier: GPL-2.0

#include <linux/math64.h>

#include "lpf/hw/lpf_hw_pwm.h"
#include "lpf_led_internal.h"

static int32_t lpf_led_pwm_init(lpf_led_context_t *ctx)
{
	lpf_pwm_config_t pwm_config;
	lpf_hw_pwm_handle_t pwm;
	int32_t ret;

	if (!ctx->config->hw.pwm.consumer ||
	    ctx->config->hw.pwm.period_ns == 0)
		return OSAL_ERR_INVALID_PARAM;

	pwm_config.consumer = ctx->config->hw.pwm.consumer;
	pwm_config.period_ns = ctx->config->hw.pwm.period_ns;
	pwm_config.duty_ns = 0;
	pwm_config.enabled = false;
	pwm_config.polarity_inversed = ctx->config->hw.pwm.polarity_inversed;

	ret = lpf_hw_pwm_init(&pwm_config, &pwm);
	if (ret != OSAL_SUCCESS)
		return ret;

	ctx->hw_handle = pwm;
	return OSAL_SUCCESS;
}

static int32_t lpf_led_pwm_apply(lpf_led_context_t *ctx)
{
	lpf_pwm_state_t state;
	uint64_t duty;

	if (!ctx->hw_handle || ctx->config->max_brightness == 0)
		return OSAL_ERR_INVALID_PARAM;

	duty = (uint64_t)ctx->config->hw.pwm.period_ns * ctx->brightness;
	duty = div_u64(duty, ctx->config->max_brightness);

	state.period_ns = ctx->config->hw.pwm.period_ns;
	state.duty_ns = (uint32_t)duty;
	state.enabled = ctx->enabled && ctx->brightness > 0;
	state.polarity_inversed = ctx->config->hw.pwm.polarity_inversed;

	return lpf_hw_pwm_apply(ctx->hw_handle, &state);
}

static void lpf_led_pwm_deinit(lpf_led_context_t *ctx)
{
	if (ctx->hw_handle)
		lpf_hw_pwm_deinit(ctx->hw_handle);
	ctx->hw_handle = NULL;
}

const lpf_led_control_ops_t lpf_led_pwm_ops = {
	.control = LPF_CONFIG_LED_CONTROL_PWM,
	.init = lpf_led_pwm_init,
	.apply = lpf_led_pwm_apply,
	.deinit = lpf_led_pwm_deinit,
};
