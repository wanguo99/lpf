// SPDX-License-Identifier: GPL-2.0

#include "lpf/hw/lpf_hw_gpio.h"
#include "lpf_led_internal.h"

static lpf_gpio_level_t lpf_led_gpio_level(const lpf_config_led_config_t *config,
					   bool enabled)
{
	bool active = enabled;

	if (config->hw.gpio.active_low)
		active = !active;

	return active ? LPF_GPIO_LEVEL_HIGH : LPF_GPIO_LEVEL_LOW;
}

static int32_t lpf_led_gpio_init(lpf_led_context_t *ctx)
{
	lpf_gpio_config_t gpio_config;

	gpio_config.direction = LPF_GPIO_DIR_OUTPUT;
	gpio_config.initial_level = lpf_led_gpio_level(ctx->config,
						       ctx->enabled);
	gpio_config.edge = LPF_GPIO_EDGE_NONE;
	gpio_config.callback = NULL;
	gpio_config.user_data = NULL;

	return lpf_hw_gpio_init(ctx->config->hw.gpio.gpio_num, &gpio_config);
}

static int32_t lpf_led_gpio_apply(lpf_led_context_t *ctx)
{
	lpf_gpio_level_t level;

	level = lpf_led_gpio_level(ctx->config, ctx->enabled);
	return lpf_hw_gpio_set_level(ctx->config->hw.gpio.gpio_num, level);
}

static void lpf_led_gpio_deinit(lpf_led_context_t *ctx)
{
	lpf_hw_gpio_deinit(ctx->config->hw.gpio.gpio_num);
}

const lpf_led_control_ops_t lpf_led_gpio_ops = {
	.control = LPF_CONFIG_LED_CONTROL_GPIO,
	.init = lpf_led_gpio_init,
	.apply = lpf_led_gpio_apply,
	.deinit = lpf_led_gpio_deinit,
};
