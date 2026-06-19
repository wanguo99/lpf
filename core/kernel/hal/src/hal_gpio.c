// SPDX-License-Identifier: GPL-2.0

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>

#include "osal.h"
#include "hal_gpio.h"
#include "hal_internal.h"

#define HAL_GPIO_MAX_PINS 1024

typedef struct {
	uint32_t gpio_num;
	int irq;
	hal_gpio_direction_t direction;
	hal_gpio_isr_callback_t callback;
	void *user_data;
	bool requested;
	bool interrupt_configured;
	bool interrupt_enabled;
} hal_gpio_context_t;

static osal_mutex_t g_hal_gpio_lock;
static hal_gpio_context_t *g_hal_gpio_table[HAL_GPIO_MAX_PINS];

static int32_t hal_gpio_errno_to_status(int ret)
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

static bool hal_gpio_valid_direction(hal_gpio_direction_t direction)
{
	return direction == HAL_GPIO_DIR_INPUT || direction == HAL_GPIO_DIR_OUTPUT;
}

static bool hal_gpio_valid_level(hal_gpio_level_t level)
{
	return level == HAL_GPIO_LEVEL_LOW || level == HAL_GPIO_LEVEL_HIGH;
}

static unsigned long hal_gpio_irq_flags(hal_gpio_edge_t edge)
{
	switch (edge) {
	case HAL_GPIO_EDGE_NONE:
		return 0;
	case HAL_GPIO_EDGE_RISING:
		return IRQF_TRIGGER_RISING;
	case HAL_GPIO_EDGE_FALLING:
		return IRQF_TRIGGER_FALLING;
	case HAL_GPIO_EDGE_BOTH:
		return IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
	default:
		return ULONG_MAX;
	}
}

static irqreturn_t hal_gpio_irq_thread(int irq, void *data)
{
	hal_gpio_context_t *ctx = data;
	hal_gpio_level_t level;
	int value;

	(void)irq;

	if (!ctx || !ctx->callback)
		return IRQ_HANDLED;

	value = gpio_get_value(ctx->gpio_num);
	level = value ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
	ctx->callback(ctx->gpio_num, level, ctx->user_data);
	return IRQ_HANDLED;
}

static void hal_gpio_free_interrupt(hal_gpio_context_t *ctx)
{
	if (!ctx || !ctx->interrupt_configured)
		return;

	free_irq(ctx->irq, ctx);
	ctx->interrupt_configured = false;
	ctx->interrupt_enabled = false;
	ctx->irq = 0;
	ctx->callback = NULL;
	ctx->user_data = NULL;
}

static int32_t hal_gpio_set_interrupt_locked(hal_gpio_context_t *ctx,
					     hal_gpio_edge_t edge,
					     hal_gpio_isr_callback_t callback,
					     void *user_data)
{
	unsigned long flags;
	int irq;
	int ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	flags = hal_gpio_irq_flags(edge);
	if (flags == ULONG_MAX || (edge != HAL_GPIO_EDGE_NONE && !callback))
		return OSAL_ERR_INVALID_PARAM;

	hal_gpio_free_interrupt(ctx);
	if (edge == HAL_GPIO_EDGE_NONE)
		return OSAL_SUCCESS;

	irq = gpio_to_irq(ctx->gpio_num);
	if (irq < 0)
		return hal_gpio_errno_to_status(irq);

	ctx->irq = irq;
	ctx->callback = callback;
	ctx->user_data = user_data;
	ret = request_threaded_irq(irq, NULL, hal_gpio_irq_thread,
				   flags | IRQF_ONESHOT, "esmw-hal-gpio", ctx);
	if (ret < 0) {
		ctx->irq = -1;
		ctx->callback = NULL;
		ctx->user_data = NULL;
		return hal_gpio_errno_to_status(ret);
	}

	ctx->interrupt_configured = true;
	ctx->interrupt_enabled = true;
	return OSAL_SUCCESS;
}

static hal_gpio_context_t *hal_gpio_get_context(uint32_t gpio_num)
{
	if (gpio_num >= HAL_GPIO_MAX_PINS)
		return NULL;

	return g_hal_gpio_table[gpio_num];
}

int32_t hal_gpio_init(uint32_t gpio_num, const hal_gpio_config_t *config)
{
	hal_gpio_context_t *ctx;
	bool new_context = false;
	int ret;

	if (!config || gpio_num >= HAL_GPIO_MAX_PINS ||
	    !hal_gpio_valid_direction(config->direction) ||
	    !hal_gpio_valid_level(config->initial_level) ||
	    hal_gpio_irq_flags(config->edge) == ULONG_MAX ||
	    (config->edge != HAL_GPIO_EDGE_NONE && !config->callback))
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = g_hal_gpio_table[gpio_num];
	if (!ctx) {
		ctx = osal_zalloc(sizeof(*ctx));
		if (!ctx) {
			osal_mutex_unlock(&g_hal_gpio_lock);
			return OSAL_ERR_NO_MEMORY;
		}

		ret = gpio_request(gpio_num, "es-middleware-hal");
		if (ret < 0) {
			osal_free(ctx);
			osal_mutex_unlock(&g_hal_gpio_lock);
			return hal_gpio_errno_to_status(ret);
		}

		ctx->gpio_num = gpio_num;
		ctx->requested = true;
		ctx->irq = -1;
		g_hal_gpio_table[gpio_num] = ctx;
		new_context = true;
	}

	if (config->direction == HAL_GPIO_DIR_INPUT)
		ret = gpio_direction_input(gpio_num);
	else
		ret = gpio_direction_output(gpio_num,
					    config->initial_level ==
						    HAL_GPIO_LEVEL_HIGH);

	if (ret < 0)
		goto err_existing;

	ctx->direction = config->direction;

	if (config->edge != HAL_GPIO_EDGE_NONE) {
		ret = hal_gpio_set_interrupt_locked(ctx, config->edge,
						    config->callback,
						    config->user_data);
		if (ret != OSAL_SUCCESS)
			goto err_existing;
	}

	osal_mutex_unlock(&g_hal_gpio_lock);
	return OSAL_SUCCESS;

err_existing:
	if (new_context && ctx && ctx->requested && !ctx->interrupt_configured) {
		gpio_free(gpio_num);
		g_hal_gpio_table[gpio_num] = NULL;
		osal_free(ctx);
	}
	osal_mutex_unlock(&g_hal_gpio_lock);
	return ret < 0 ? hal_gpio_errno_to_status(ret) : ret;
}
EXPORT_SYMBOL_GPL(hal_gpio_init);

int32_t hal_gpio_deinit(uint32_t gpio_num)
{
	hal_gpio_context_t *ctx;

	if (gpio_num >= HAL_GPIO_MAX_PINS)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = g_hal_gpio_table[gpio_num];
	if (!ctx) {
		osal_mutex_unlock(&g_hal_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	hal_gpio_free_interrupt(ctx);
	gpio_free(gpio_num);
	g_hal_gpio_table[gpio_num] = NULL;
	osal_mutex_unlock(&g_hal_gpio_lock);
	osal_free(ctx);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_gpio_deinit);

int32_t hal_gpio_set_direction(uint32_t gpio_num,
			       hal_gpio_direction_t direction)
{
	hal_gpio_context_t *ctx;
	int ret;

	if (gpio_num >= HAL_GPIO_MAX_PINS || !hal_gpio_valid_direction(direction))
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = hal_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_hal_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	if (direction == HAL_GPIO_DIR_INPUT)
		ret = gpio_direction_input(gpio_num);
	else
		ret = gpio_direction_output(gpio_num, 0);

	if (ret == 0)
		ctx->direction = direction;
	osal_mutex_unlock(&g_hal_gpio_lock);
	return hal_gpio_errno_to_status(ret);
}
EXPORT_SYMBOL_GPL(hal_gpio_set_direction);

int32_t hal_gpio_get_direction(uint32_t gpio_num,
			       hal_gpio_direction_t *direction)
{
	hal_gpio_context_t *ctx;

	if (gpio_num >= HAL_GPIO_MAX_PINS || !direction)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = hal_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_hal_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	*direction = ctx->direction;
	osal_mutex_unlock(&g_hal_gpio_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_gpio_get_direction);

int32_t hal_gpio_set_level(uint32_t gpio_num, hal_gpio_level_t level)
{
	hal_gpio_context_t *ctx;

	if (gpio_num >= HAL_GPIO_MAX_PINS || !hal_gpio_valid_level(level))
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = hal_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_hal_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	gpio_set_value(gpio_num, level == HAL_GPIO_LEVEL_HIGH);
	osal_mutex_unlock(&g_hal_gpio_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_gpio_set_level);

int32_t hal_gpio_get_level(uint32_t gpio_num, hal_gpio_level_t *level)
{
	hal_gpio_context_t *ctx;
	int value;

	if (gpio_num >= HAL_GPIO_MAX_PINS || !level)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = hal_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_hal_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	value = gpio_get_value(gpio_num);
	*level = value ? HAL_GPIO_LEVEL_HIGH : HAL_GPIO_LEVEL_LOW;
	osal_mutex_unlock(&g_hal_gpio_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_gpio_get_level);

int32_t hal_gpio_set_interrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
			       hal_gpio_isr_callback_t callback,
			       void *user_data)
{
	hal_gpio_context_t *ctx;
	int ret;

	if (gpio_num >= HAL_GPIO_MAX_PINS ||
	    hal_gpio_irq_flags(edge) == ULONG_MAX ||
	    (edge != HAL_GPIO_EDGE_NONE && !callback))
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = hal_gpio_get_context(gpio_num);
	if (!ctx) {
		osal_mutex_unlock(&g_hal_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	ret = hal_gpio_set_interrupt_locked(ctx, edge, callback, user_data);
	osal_mutex_unlock(&g_hal_gpio_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(hal_gpio_set_interrupt);

int32_t hal_gpio_enable_interrupt(uint32_t gpio_num)
{
	hal_gpio_context_t *ctx;

	if (gpio_num >= HAL_GPIO_MAX_PINS)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = hal_gpio_get_context(gpio_num);
	if (!ctx || !ctx->interrupt_configured) {
		osal_mutex_unlock(&g_hal_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	if (!ctx->interrupt_enabled) {
		enable_irq(ctx->irq);
		ctx->interrupt_enabled = true;
	}
	osal_mutex_unlock(&g_hal_gpio_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_gpio_enable_interrupt);

int32_t hal_gpio_disable_interrupt(uint32_t gpio_num)
{
	hal_gpio_context_t *ctx;

	if (gpio_num >= HAL_GPIO_MAX_PINS)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_hal_gpio_lock);
	ctx = hal_gpio_get_context(gpio_num);
	if (!ctx || !ctx->interrupt_configured) {
		osal_mutex_unlock(&g_hal_gpio_lock);
		return OSAL_ERR_INVALID_ID;
	}

	if (ctx->interrupt_enabled) {
		disable_irq_nosync(ctx->irq);
		ctx->interrupt_enabled = false;
	}
	osal_mutex_unlock(&g_hal_gpio_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_gpio_disable_interrupt);

int hal_gpio_module_init(void)
{
	return osal_mutex_init(&g_hal_gpio_lock, NULL);
}

void hal_gpio_module_deinit(void)
{
	osal_mutex_destroy(&g_hal_gpio_lock);
}
