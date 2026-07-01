// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_led_gpio.c
 * @brief GPIO backend for PDM LED devices
 */

#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>

#include "pdm/registry/pdm_backend.h"
#include "pdm/pdm_led.h"
#include "pdm_led_internal.h"
#include "pdm/log/pdm_log.h"

static int pdm_led_gpio_setup(struct pdm_led_instance *inst)
{
	struct device *dev = pdm_device_to_dev(inst->base.pdm_dev);
	struct gpio_desc *gpiod;

	gpiod = gpiod_get(dev, "led", GPIOD_OUT_LOW);
	if (PTR_ERR(gpiod) == -ENOENT) {
		gpiod = gpiod_get(dev, NULL, GPIOD_OUT_LOW);
	}
	if (IS_ERR(gpiod)) {
		LOG_ERROR("Failed to get GPIO for %s: %ld",
			  dev_name(dev), PTR_ERR(gpiod));
		return PTR_ERR(gpiod);
	}

	inst->hw.gpiod = gpiod;
	return 0;
}

static void pdm_led_gpio_cleanup(struct pdm_led_instance *inst)
{
	if (!inst->hw.gpiod) {
		return;
	}

	gpiod_set_value_cansleep(inst->hw.gpiod, 0);
	gpiod_put(inst->hw.gpiod);
	inst->hw.gpiod = NULL;
}

static int pdm_led_gpio_apply(struct pdm_led_instance *inst)
{
	int value;

	if (!inst->hw.gpiod) {
		return -ENODEV;
	}

	value = inst->enabled && inst->brightness ? 1 : 0;
	gpiod_set_value_cansleep(inst->hw.gpiod, value);
	return 0;
}

static const struct of_device_id pdm_led_gpio_of_match[] = {
	{ .compatible = "pdm,led-gpio" },
	{ .compatible = "vendor,pdm-led-gpio" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_led_gpio_of_match);

static const struct pdm_led_backend_ops pdm_led_gpio_ops = {
	.type = PDM_LED_BACKEND_GPIO,
	.name = "gpio",
	.capability = PDM_LED_CAP_GPIO,
	.setup = pdm_led_gpio_setup,
	.cleanup = pdm_led_gpio_cleanup,
	.apply = pdm_led_gpio_apply,
};

pdm_backend_register(led_gpio, PDM_LED_DEVICE_TYPE,
		     PDM_BACKEND_CLASS_CONTROL, pdm_led_gpio_of_match,
		     &pdm_led_gpio_ops, NULL, NULL);
