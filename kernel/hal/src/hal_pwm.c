// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/pwm.h>

#include "osal.h"
#include "hal_pwm.h"

typedef struct {
	struct pwm_device *pwm;
	osal_mutex_t lock;
} hal_pwm_context_t;

static int32_t hal_pwm_errno_to_status(int ret)
{
	int err;

	if (ret >= 0)
		return OSAL_SUCCESS;

	err = -ret;
	if (err == ENODEV || err == ENOENT)
		return OSAL_ENOENT;
	if (err == EOPNOTSUPP || err == ENOSYS)
		return OSAL_ERR_NOT_SUPPORTED;
	if (err == EBUSY)
		return OSAL_ERR_BUSY;

	return err;
}

static void hal_pwm_fill_state(const struct pwm_state *src,
			       hal_pwm_state_t *dst)
{
	dst->period_ns = (uint32_t)src->period;
	dst->duty_ns = (uint32_t)src->duty_cycle;
	dst->enabled = src->enabled;
	dst->polarity_inversed = src->polarity == PWM_POLARITY_INVERSED;
}

static void hal_pwm_fill_kernel_state(const hal_pwm_state_t *src,
				      struct pwm_state *dst)
{
	dst->period = src->period_ns;
	dst->duty_cycle = src->duty_ns;
	dst->enabled = src->enabled;
	dst->polarity = src->polarity_inversed ? PWM_POLARITY_INVERSED :
						 PWM_POLARITY_NORMAL;
}

int32_t hal_pwm_init(const hal_pwm_config_t *config, hal_pwm_handle_t *handle)
{
	hal_pwm_context_t *ctx;
	struct pwm_state state;
	int ret;

	if (!config || !handle || config->period_ns == 0 ||
	    config->duty_ns > config->period_ns)
		return OSAL_ERR_INVALID_PARAM;

	ctx = osal_zalloc(sizeof(*ctx));
	if (!ctx)
		return OSAL_ERR_NO_MEMORY;

	if (osal_mutex_init(&ctx->lock, NULL) != OSAL_SUCCESS) {
		osal_free(ctx);
		return OSAL_ERR_GENERIC;
	}

	ctx->pwm = pwm_get(NULL, config->consumer);
	if (IS_ERR(ctx->pwm)) {
		ret = PTR_ERR(ctx->pwm);
		osal_mutex_destroy(&ctx->lock);
		osal_free(ctx);
		return hal_pwm_errno_to_status(ret);
	}

	pwm_init_state(ctx->pwm, &state);
	state.period = config->period_ns;
	state.duty_cycle = config->duty_ns;
	state.enabled = config->enabled;
	state.polarity = config->polarity_inversed ? PWM_POLARITY_INVERSED :
						     PWM_POLARITY_NORMAL;

	ret = pwm_apply_might_sleep(ctx->pwm, &state);
	if (ret < 0) {
		pwm_put(ctx->pwm);
		osal_mutex_destroy(&ctx->lock);
		osal_free(ctx);
		return hal_pwm_errno_to_status(ret);
	}

	*handle = ctx;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_pwm_init);

int32_t hal_pwm_deinit(hal_pwm_handle_t handle)
{
	hal_pwm_context_t *ctx = handle;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	pwm_disable(ctx->pwm);
	pwm_put(ctx->pwm);
	osal_mutex_unlock(&ctx->lock);
	osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_pwm_deinit);

int32_t hal_pwm_apply(hal_pwm_handle_t handle, const hal_pwm_state_t *state)
{
	hal_pwm_context_t *ctx = handle;
	struct pwm_state pwm_state;
	int ret;

	if (!ctx || !state || state->period_ns == 0 ||
	    state->duty_ns > state->period_ns)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	pwm_get_state(ctx->pwm, &pwm_state);
	hal_pwm_fill_kernel_state(state, &pwm_state);
	ret = pwm_apply_might_sleep(ctx->pwm, &pwm_state);
	osal_mutex_unlock(&ctx->lock);

	return hal_pwm_errno_to_status(ret);
}
EXPORT_SYMBOL_GPL(hal_pwm_apply);

int32_t hal_pwm_get_state(hal_pwm_handle_t handle, hal_pwm_state_t *state)
{
	hal_pwm_context_t *ctx = handle;
	struct pwm_state pwm_state;

	if (!ctx || !state)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	pwm_get_state(ctx->pwm, &pwm_state);
	osal_mutex_unlock(&ctx->lock);

	hal_pwm_fill_state(&pwm_state, state);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_pwm_get_state);

int32_t hal_pwm_enable(hal_pwm_handle_t handle)
{
	hal_pwm_context_t *ctx = handle;
	int ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ret = pwm_enable(ctx->pwm);
	osal_mutex_unlock(&ctx->lock);

	return hal_pwm_errno_to_status(ret);
}
EXPORT_SYMBOL_GPL(hal_pwm_enable);

int32_t hal_pwm_disable(hal_pwm_handle_t handle)
{
	hal_pwm_context_t *ctx = handle;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	pwm_disable(ctx->pwm);
	osal_mutex_unlock(&ctx->lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_pwm_disable);
