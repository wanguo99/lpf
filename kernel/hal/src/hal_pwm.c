// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "hal_pwm.h"
#include "lpf/lpf_soc_adapter.h"

typedef struct {
	lpf_pwm_handle_t pwm;
	osal_mutex_t lock;
} hal_pwm_context_t;

static void hal_pwm_fill_state(const lpf_pwm_state_t *src, hal_pwm_state_t *dst)
{
	dst->period_ns = src->period_ns;
	dst->duty_ns = src->duty_ns;
	dst->enabled = src->enabled;
	dst->polarity_inversed = src->polarity_inversed;
}

static void hal_pwm_fill_lpf_state(const hal_pwm_state_t *src,
				   lpf_pwm_state_t *dst)
{
	dst->period_ns = src->period_ns;
	dst->duty_ns = src->duty_ns;
	dst->enabled = src->enabled;
	dst->polarity_inversed = src->polarity_inversed;
}

int32_t hal_pwm_init(const hal_pwm_config_t *config, hal_pwm_handle_t *handle)
{
	hal_pwm_context_t *ctx;
	lpf_pwm_state_t state;
	int32_t ret;

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

	ret = lpf_soc_pwm_get(config->consumer, &ctx->pwm);
	if (ret != OSAL_SUCCESS) {
		osal_mutex_destroy(&ctx->lock);
		osal_free(ctx);
		return ret;
	}

	state.period_ns = config->period_ns;
	state.duty_ns = config->duty_ns;
	state.enabled = config->enabled;
	state.polarity_inversed = config->polarity_inversed;

	ret = lpf_soc_pwm_apply(ctx->pwm, &state);
	if (ret != OSAL_SUCCESS) {
		lpf_soc_pwm_put(ctx->pwm);
		osal_mutex_destroy(&ctx->lock);
		osal_free(ctx);
		return ret;
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
	lpf_soc_pwm_disable(ctx->pwm);
	lpf_soc_pwm_put(ctx->pwm);
	osal_mutex_unlock(&ctx->lock);
	osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_pwm_deinit);

int32_t hal_pwm_apply(hal_pwm_handle_t handle, const hal_pwm_state_t *state)
{
	hal_pwm_context_t *ctx = handle;
	lpf_pwm_state_t pwm_state;
	int32_t ret;

	if (!ctx || !state || state->period_ns == 0 ||
	    state->duty_ns > state->period_ns)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	hal_pwm_fill_lpf_state(state, &pwm_state);
	ret = lpf_soc_pwm_apply(ctx->pwm, &pwm_state);
	osal_mutex_unlock(&ctx->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(hal_pwm_apply);

int32_t hal_pwm_get_state(hal_pwm_handle_t handle, hal_pwm_state_t *state)
{
	hal_pwm_context_t *ctx = handle;
	lpf_pwm_state_t pwm_state;
	int32_t ret;

	if (!ctx || !state)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ret = lpf_soc_pwm_get_state(ctx->pwm, &pwm_state);
	osal_mutex_unlock(&ctx->lock);
	if (ret != OSAL_SUCCESS)
		return ret;

	hal_pwm_fill_state(&pwm_state, state);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_pwm_get_state);

int32_t hal_pwm_enable(hal_pwm_handle_t handle)
{
	hal_pwm_context_t *ctx = handle;
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ret = lpf_soc_pwm_enable(ctx->pwm);
	osal_mutex_unlock(&ctx->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(hal_pwm_enable);

int32_t hal_pwm_disable(hal_pwm_handle_t handle)
{
	hal_pwm_context_t *ctx = handle;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	lpf_soc_pwm_disable(ctx->pwm);
	osal_mutex_unlock(&ctx->lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_pwm_disable);
