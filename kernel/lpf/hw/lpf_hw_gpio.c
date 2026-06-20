// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/lpf_hw_gpio.h"
#include "lpf_hw_internal.h"
#include "lpf/lpf_soc_adapter.h"

#define LPF_HW_GPIO_MAX_PINS 1024

typedef struct {
	uint32_t gpio_num;
	void *irq_handle;
	lpf_gpio_direction_t direction;
	lpf_gpio_irq_callback_t callback;
	void *user_data;
	bool requested;
	bool interrupt_configured;
	bool interrupt_enabled;
} lpf_hw_gpio_context_t;

static osal_mutex_t g_lpf_hw_gpio_lock;
static lpf_hw_gpio_context_t *g_lpf_hw_gpio_table[LPF_HW_GPIO_MAX_PINS];

static bool lpf_hw_gpio_valid_direction(lpf_gpio_direction_t direction)
{
	return direction == LPF_GPIO_DIR_INPUT || direction == LPF_GPIO_DIR_OUTPUT;
}

static bool lpf_hw_gpio_valid_level(lpf_gpio_level_t level)
{
	return level == LPF_GPIO_LEVEL_LOW || level == LPF_GPIO_LEVEL_HIGH;
}

static bool lpf_hw_gpio_valid_edge(lpf_gpio_edge_t edge)
{
	switch (edge) {
	case LPF_GPIO_EDGE_NONE:
	case LPF_GPIO_EDGE_RISING:
	case LPF_GPIO_EDGE_FALLING:
	case LPF_GPIO_EDGE_BOTH:
		return true;
	default:
		return false;
	}
}

static lpf_gpio_direction_t
lpf_hw_gpio_to_lpf_direction(lpf_gpio_direction_t direction)
{
	return direction == LPF_GPIO_DIR_INPUT ? LPF_GPIO_DIR_INPUT :
						 LPF_GPIO_DIR_OUTPUT;
}

static lpf_gpio_level_t lpf_hw_gpio_to_lpf_level(lpf_gpio_level_t level)
{
	return level == LPF_GPIO_LEVEL_HIGH ? LPF_GPIO_LEVEL_HIGH :
					      LPF_GPIO_LEVEL_LOW;
}

static lpf_gpio_level_t lpf_hw_gpio_from_lpf_level(lpf_gpio_level_t level)
{
	return level == LPF_GPIO_LEVEL_HIGH ? LPF_GPIO_LEVEL_HIGH :
					      LPF_GPIO_LEVEL_LOW;
}

static lpf_gpio_edge_t lpf_hw_gpio_to_lpf_edge(lpf_gpio_edge_t edge)
{
	switch (edge) {
	case LPF_GPIO_EDGE_NONE:
		return LPF_GPIO_EDGE_NONE;
	case LPF_GPIO_EDGE_RISING:
		return LPF_GPIO_EDGE_RISING;
	case LPF_GPIO_EDGE_FALLING:
		return LPF_GPIO_EDGE_FALLING;
	case LPF_GPIO_EDGE_BOTH:
		return LPF_GPIO_EDGE_BOTH;
	default:
		return LPF_GPIO_EDGE_NONE;
	}
}

static void lpf_hw_gpio_irq_callback(uint32_t gpio_num,
				  lpf_gpio_level_t lpf_level,
				  void *data)
{
	lpf_hw_gpio_context_t *ctx = data;
	lpf_gpio_level_t level;

	if (!ctx || !ctx->callback)
		return;

	level = lpf_hw_gpio_from_lpf_level(lpf_level);
	ctx->callback(gpio_num, level, ctx->user_data);
}

static void lpf_hw_gpio_free_interrupt(lpf_hw_gpio_context_t *ctx)
{
	if (!ctx || !ctx->interrupt_configured)
		return;

	lpf_soc_gpio_free_interrupt(ctx->irq_handle);
	ctx->interrupt_configured = false;
	ctx->interrupt_enabled = false;
	ctx->irq_handle = NULL;
	ctx->callback = NULL;
	ctx->user_data = NULL;
}

static int32_t lpf_hw_gpio_set_interrupt_locked(lpf_hw_gpio_context_t *ctx,
					     lpf_gpio_edge_t edge,
					     lpf_gpio_irq_callback_t callback,
					     void *user_data)
{
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	if (!lpf_hw_gpio_valid_edge(edge) ||
	    (edge != LPF_GPIO_EDGE_NONE && !callback))
		return OSAL_ERR_INVALID_PARAM;

	lpf_hw_gpio_free_interrupt(ctx);
	if (edge == LPF_GPIO_EDGE_NONE)
		return OSAL_SUCCESS;

	ctx->callback = callback;
	ctx->user_data = user_data;
	ret = lpf_soc_gpio_request_interrupt(ctx->gpio_num,
					     lpf_hw_gpio_to_lpf_edge(edge),
					     lpf_hw_gpio_irq_callback, ctx,
					     &ctx->irq_handle);
	if (ret != OSAL_SUCCESS) {
		ctx->callback = NULL;
		ctx->user_data = NULL;
		return ret;
	}

	ctx->interrupt_configured = true;
	ctx->interrupt_enabled = true;
	return OSAL_SUCCESS;
}

static lpf_hw_gpio_context_t *lpf_hw_gpio_get_context(uint32_t gpio_num)
{
	if (gpio_num >= LPF_HW_GPIO_MAX_PINS)
		return NULL;

	return g_lpf_hw_gpio_table[gpio_num];
}

int32_t lpf_hw_gpio_init(uint32_t gpio_num, const lpf_gpio_config_t *config)
{
	lpf_hw_gpio_context_t *ctx;
	bool new_context = false;
	int ret;

	if (!config || gpio_num >= LPF_HW_GPIO_MAX_PINS ||
	    !lpf_hw_gpio_valid_direction(config->direction) ||
	    !lpf_hw_gpio_valid_level(config->initial_level) ||
	    !lpf_hw_gpio_valid_edge(config->edge) ||
	    (config->edge != LPF_GPIO_EDGE_NONE && !config->callback))
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = g_lpf_hw_gpio_table[gpio_num];
	if (!ctx) {
		ctx = osal_zalloc(sizeof(*ctx));
		if (!ctx) {
			osal_mutex_unlock(&g_lpf_hw_gpio_lock);
			return OSAL_ERR_NO_MEMORY;
		}

		ret = lpf_soc_gpio_request(gpio_num, "lpf-hw");
		if (ret != OSAL_SUCCESS) {
			osal_free(ctx);
			osal_mutex_unlock(&g_lpf_hw_gpio_lock);
			return ret;
		}

		ctx->gpio_num = gpio_num;
		ctx->requested = true;
		g_lpf_hw_gpio_table[gpio_num] = ctx;
		new_context = true;
	}

	ret = lpf_soc_gpio_set_direction(
		gpio_num, lpf_hw_gpio_to_lpf_direction(config->direction),
		lpf_hw_gpio_to_lpf_level(config->initial_level));
	if (ret != OSAL_SUCCESS)
		goto err_existing;

	ctx->direction = config->direction;

	if (config->edge != LPF_GPIO_EDGE_NONE) {
		ret = lpf_hw_gpio_set_interrupt_locked(ctx, config->edge,
						    config->callback,
						    config->user_data);
		if (ret != OSAL_SUCCESS)
			goto err_existing;
	}

	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return OSAL_SUCCESS;

err_existing:
	if (new_context && ctx && ctx->requested && !ctx->interrupt_configured) {
		lpf_soc_gpio_free(gpio_num);
		g_lpf_hw_gpio_table[gpio_num] = NULL;
		osal_free(ctx);
	}
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_init);

int32_t lpf_hw_gpio_deinit(uint32_t gpio_num)
{
	lpf_hw_gpio_context_t *ctx;

	if (gpio_num >= LPF_HW_GPIO_MAX_PINS)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = g_lpf_hw_gpio_table[gpio_num];
	if (!ctx) {
		osal_mutex_unlock(&g_lpf_hw_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	lpf_hw_gpio_free_interrupt(ctx);
	lpf_soc_gpio_free(gpio_num);
	g_lpf_hw_gpio_table[gpio_num] = NULL;
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	osal_free(ctx);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_deinit);

int32_t lpf_hw_gpio_set_direction(uint32_t gpio_num,
			       lpf_gpio_direction_t direction)
{
	lpf_hw_gpio_context_t *ctx;
	int ret;

	if (gpio_num >= LPF_HW_GPIO_MAX_PINS || !lpf_hw_gpio_valid_direction(direction))
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = lpf_hw_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_lpf_hw_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	ret = lpf_soc_gpio_set_direction(
		gpio_num, lpf_hw_gpio_to_lpf_direction(direction),
		LPF_GPIO_LEVEL_LOW);

	if (ret == OSAL_SUCCESS)
		ctx->direction = direction;
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_set_direction);

int32_t lpf_hw_gpio_get_direction(uint32_t gpio_num,
			       lpf_gpio_direction_t *direction)
{
	lpf_hw_gpio_context_t *ctx;

	if (gpio_num >= LPF_HW_GPIO_MAX_PINS || !direction)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = lpf_hw_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_lpf_hw_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	*direction = ctx->direction;
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_get_direction);

int32_t lpf_hw_gpio_set_level(uint32_t gpio_num, lpf_gpio_level_t level)
{
	lpf_hw_gpio_context_t *ctx;
	int32_t ret;

	if (gpio_num >= LPF_HW_GPIO_MAX_PINS || !lpf_hw_gpio_valid_level(level))
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = lpf_hw_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_lpf_hw_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	ret = lpf_soc_gpio_set_level(gpio_num, lpf_hw_gpio_to_lpf_level(level));
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_set_level);

int32_t lpf_hw_gpio_get_level(uint32_t gpio_num, lpf_gpio_level_t *level)
{
	lpf_hw_gpio_context_t *ctx;
	lpf_gpio_level_t lpf_level;
	int32_t ret;

	if (gpio_num >= LPF_HW_GPIO_MAX_PINS || !level)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = lpf_hw_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_lpf_hw_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	ret = lpf_soc_gpio_get_level(gpio_num, &lpf_level);
	if (ret == OSAL_SUCCESS)
		*level = lpf_hw_gpio_from_lpf_level(lpf_level);
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_get_level);

int32_t lpf_hw_gpio_set_interrupt(uint32_t gpio_num, lpf_gpio_edge_t edge,
			       lpf_gpio_irq_callback_t callback,
			       void *user_data)
{
	lpf_hw_gpio_context_t *ctx;
	int ret;

	if (gpio_num >= LPF_HW_GPIO_MAX_PINS ||
	    !lpf_hw_gpio_valid_edge(edge) ||
	    (edge != LPF_GPIO_EDGE_NONE && !callback))
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = lpf_hw_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_lpf_hw_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	ret = lpf_hw_gpio_set_interrupt_locked(ctx, edge, callback, user_data);
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_set_interrupt);

int32_t lpf_hw_gpio_enable_interrupt(uint32_t gpio_num)
{
	lpf_hw_gpio_context_t *ctx;
	int32_t ret;

	if (gpio_num >= LPF_HW_GPIO_MAX_PINS)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = lpf_hw_gpio_get_context(gpio_num);
	if (!ctx || !ctx->interrupt_configured) {
		osal_mutex_unlock(&g_lpf_hw_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	if (!ctx->interrupt_enabled) {
		ret = lpf_soc_gpio_enable_interrupt(ctx->irq_handle);
		if (ret != OSAL_SUCCESS) {
			osal_mutex_unlock(&g_lpf_hw_gpio_lock);
			return ret;
		}
		ctx->interrupt_enabled = true;
	}
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_enable_interrupt);

int32_t lpf_hw_gpio_disable_interrupt(uint32_t gpio_num)
{
	lpf_hw_gpio_context_t *ctx;
	int32_t ret;

	if (gpio_num >= LPF_HW_GPIO_MAX_PINS)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_hw_gpio_lock);
	ctx = lpf_hw_gpio_get_context(gpio_num);
	if (!ctx || !ctx->interrupt_configured) {
		osal_mutex_unlock(&g_lpf_hw_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	if (ctx->interrupt_enabled) {
		ret = lpf_soc_gpio_disable_interrupt(ctx->irq_handle);
		if (ret != OSAL_SUCCESS) {
			osal_mutex_unlock(&g_lpf_hw_gpio_lock);
			return ret;
		}
		ctx->interrupt_enabled = false;
	}
	osal_mutex_unlock(&g_lpf_hw_gpio_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_hw_gpio_disable_interrupt);

static int lpf_hw_gpio_module_init(void)
{
	return osal_mutex_init(&g_lpf_hw_gpio_lock, NULL);
}

static void lpf_hw_gpio_module_deinit(void)
{
	osal_mutex_destroy(&g_lpf_hw_gpio_lock);
}

LPF_HW_BUILTIN_DRIVER(gpio, lpf_hw_gpio_module_init,
		      lpf_hw_gpio_module_deinit);
