// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_led_pwm.c
 * @brief PWM backend for PDM LED devices
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/pwm.h>

#include "pdm/core/pdm_backend.h"
#include "pdm_led_internal.h"
#include "osal.h"

static const struct of_device_id pdm_led_pwm_of_match[] = {
	{ .compatible = "pdm,led-pwm" },
	{ .compatible = "vendor,pdm-led-pwm" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_led_pwm_of_match);

static int pdm_led_pwm_setup(struct pdm_led_instance *inst)
{
	struct device *dev = pdm_device_to_dev(inst->pdm_dev);
	struct pwm_device *pwmdev;

	pwmdev = pwm_get(dev, NULL);
	if (IS_ERR(pwmdev)) {
		LOG_ERROR("PDM-LED-PWM", "Failed to get PWM for %s: %ld",
			  dev_name(dev), PTR_ERR(pwmdev));
		return PTR_ERR(pwmdev);
	}

	inst->hw.pwmdev = pwmdev;
	return 0;
}

static void pdm_led_pwm_cleanup(struct pdm_led_instance *inst)
{
	struct pwm_state state;

	if (!inst->hw.pwmdev)
		return;

	pwm_get_state(inst->hw.pwmdev, &state);
	state.enabled = false;
	state.duty_cycle = 0;
	pwm_apply_might_sleep(inst->hw.pwmdev, &state);
	pwm_put(inst->hw.pwmdev);
	inst->hw.pwmdev = NULL;
}

static int pdm_led_pwm_apply(struct pdm_led_instance *inst)
{
	struct pwm_state state;
	int ret;

	if (!inst->hw.pwmdev)
		return -ENODEV;

	pwm_init_state(inst->hw.pwmdev, &state);
	if (!state.period)
		return -EINVAL;

	if (!inst->enabled || !inst->brightness) {
		state.enabled = false;
		state.duty_cycle = 0;
		return pwm_apply_might_sleep(inst->hw.pwmdev, &state);
	}

	state.enabled = true;
	ret = pwm_set_relative_duty_cycle(&state, inst->brightness,
					    inst->max_brightness);
	if (ret)
		return ret;

	return pwm_apply_might_sleep(inst->hw.pwmdev, &state);
}

const struct pdm_led_backend_ops pdm_led_pwm_ops = {
	.type = PDM_LED_BACKEND_PWM,
	.name = "pwm",
	.capability = PDM_CTL_DEVICE_CAP_CONTROL_PWM,
	.setup = pdm_led_pwm_setup,
	.cleanup = pdm_led_pwm_cleanup,
	.apply = pdm_led_pwm_apply,
};

pdm_backend_register(led_pwm, PDM_CTL_DEVICE_TYPE_LED,
		     PDM_BACKEND_CLASS_CONTROL, pdm_led_pwm_of_match,
		     &pdm_led_pwm_ops, NULL, NULL);
