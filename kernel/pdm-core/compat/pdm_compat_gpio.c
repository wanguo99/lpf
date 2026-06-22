// SPDX-License-Identifier: GPL-2.0

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>

#include "lpf/compat/lpf_compat_errno.h"
#include "lpf/compat/lpf_compat_gpio.h"

typedef struct {
	uint32_t gpio_num;
	int irq;
	lpf_gpio_irq_callback_t callback;
	void *user_data;
	bool enabled;
} lpf_compat_gpio_irq_t;

static unsigned long lpf_compat_gpio_irq_flags(lpf_gpio_edge_t edge)
{
	switch (edge) {
	case LPF_GPIO_EDGE_NONE:
		return 0;
	case LPF_GPIO_EDGE_RISING:
		return IRQF_TRIGGER_RISING;
	case LPF_GPIO_EDGE_FALLING:
		return IRQF_TRIGGER_FALLING;
	case LPF_GPIO_EDGE_BOTH:
		return IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
	default:
		return ULONG_MAX;
	}
}

static irqreturn_t lpf_compat_gpio_irq_thread(int irq, void *data)
{
	lpf_compat_gpio_irq_t *ctx = data;
	lpf_gpio_level_t level;
	int value;

	(void)irq;

	if (!ctx || !ctx->callback)
		return IRQ_HANDLED;

	value = gpio_get_value(ctx->gpio_num);
	level = value ? LPF_GPIO_LEVEL_HIGH : LPF_GPIO_LEVEL_LOW;
	ctx->callback(ctx->gpio_num, level, ctx->user_data);
	return IRQ_HANDLED;
}

int32_t lpf_compat_gpio_request(uint32_t gpio_num, const char *label)
{
	return lpf_compat_errno_to_status(
		gpio_request(gpio_num, label ? label : "lpf-gpio"));
}

void lpf_compat_gpio_free(uint32_t gpio_num)
{
	gpio_free(gpio_num);
}

int32_t lpf_compat_gpio_set_direction(uint32_t gpio_num,
				      lpf_gpio_direction_t direction,
				      lpf_gpio_level_t initial_level)
{
	int ret;

	switch (direction) {
	case LPF_GPIO_DIR_INPUT:
		ret = gpio_direction_input(gpio_num);
		break;
	case LPF_GPIO_DIR_OUTPUT:
		ret = gpio_direction_output(gpio_num,
					    initial_level ==
						    LPF_GPIO_LEVEL_HIGH);
		break;
	default:
		return OSAL_ERR_INVALID_PARAM;
	}

	return lpf_compat_errno_to_status(ret);
}

int32_t lpf_compat_gpio_set_level(uint32_t gpio_num, lpf_gpio_level_t level)
{
	if (level != LPF_GPIO_LEVEL_LOW && level != LPF_GPIO_LEVEL_HIGH)
		return OSAL_ERR_INVALID_PARAM;

	gpio_set_value(gpio_num, level == LPF_GPIO_LEVEL_HIGH);
	return OSAL_SUCCESS;
}

int32_t lpf_compat_gpio_get_level(uint32_t gpio_num, lpf_gpio_level_t *level)
{
	int value;

	if (!level)
		return OSAL_ERR_INVALID_PARAM;

	value = gpio_get_value(gpio_num);
	*level = value ? LPF_GPIO_LEVEL_HIGH : LPF_GPIO_LEVEL_LOW;
	return OSAL_SUCCESS;
}

int32_t lpf_compat_gpio_request_interrupt(
	uint32_t gpio_num, lpf_gpio_edge_t edge,
	lpf_gpio_irq_callback_t callback, void *user_data,
	void **irq_handle)
{
	lpf_compat_gpio_irq_t *ctx;
	unsigned long flags;
	int irq;
	int ret;

	if (!irq_handle || !callback)
		return OSAL_ERR_INVALID_PARAM;

	*irq_handle = NULL;
	flags = lpf_compat_gpio_irq_flags(edge);
	if (flags == ULONG_MAX || flags == 0)
		return OSAL_ERR_INVALID_PARAM;

	irq = gpio_to_irq(gpio_num);
	if (irq < 0)
		return lpf_compat_errno_to_status(irq);

	ctx = osal_zalloc(sizeof(*ctx));
	if (!ctx)
		return OSAL_ERR_NO_MEMORY;

	ctx->gpio_num = gpio_num;
	ctx->irq = irq;
	ctx->callback = callback;
	ctx->user_data = user_data;

	ret = request_threaded_irq(irq, NULL, lpf_compat_gpio_irq_thread,
				   flags | IRQF_ONESHOT, "lpf-gpio", ctx);
	if (ret < 0) {
		osal_free(ctx);
		return lpf_compat_errno_to_status(ret);
	}

	ctx->enabled = true;
	*irq_handle = ctx;
	return OSAL_SUCCESS;
}

void lpf_compat_gpio_free_interrupt(void *irq_handle)
{
	lpf_compat_gpio_irq_t *ctx = irq_handle;

	if (!ctx)
		return;

	free_irq(ctx->irq, ctx);
	osal_free(ctx);
}

int32_t lpf_compat_gpio_enable_interrupt(void *irq_handle)
{
	lpf_compat_gpio_irq_t *ctx = irq_handle;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	if (!ctx->enabled) {
		enable_irq(ctx->irq);
		ctx->enabled = true;
	}

	return OSAL_SUCCESS;
}

int32_t lpf_compat_gpio_disable_interrupt(void *irq_handle)
{
	lpf_compat_gpio_irq_t *ctx = irq_handle;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	if (ctx->enabled) {
		disable_irq_nosync(ctx->irq);
		ctx->enabled = false;
	}

	return OSAL_SUCCESS;
}
