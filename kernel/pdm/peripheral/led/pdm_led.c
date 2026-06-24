// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_led.c
 * @brief PDM LED bus driver with memory, GPIO and PWM backends
 */

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "pdm_led_internal.h"

#include "pdm/core/driver/pdm_backend.h"
#include "pdm/core/bus/pdm_bus.h"
#include "pdm/pdm_ctl.h"
#include "pdm/pdm_led.h"
#include "osal.h"

static atomic_t pdm_led_device_count = ATOMIC_INIT(0);

static int pdm_led_memory_setup(struct pdm_led_instance *inst)
{
	(void)inst;
	return 0;
}

static void pdm_led_memory_cleanup(struct pdm_led_instance *inst)
{
	(void)inst;
}

static int pdm_led_memory_apply(struct pdm_led_instance *inst)
{
	(void)inst;
	return 0;
}

static bool pdm_led_is_generic_compatible(const char *compatible)
{
	return !compatible || !strcmp(compatible, "pdm,led") ||
	       !strcmp(compatible, "vendor,pdm-led");
}

static bool pdm_led_match(const struct pdm_device *pdm_dev)
{
	if (!pdm_dev) {
		return false;
	}
	if (pdm_led_is_generic_compatible(pdm_dev->compatible)) {
		return true;
	}

	return pdm_backend_find(PDM_CTL_DEVICE_TYPE_LED,
				PDM_BACKEND_CLASS_CONTROL,
				pdm_dev->compatible) != NULL;
}

static int pdm_led_read_dt_config(struct pdm_led_instance *inst)
{
	struct device_node *np = inst->pdm_dev->dev.of_node;
	const char *default_state;
	u32 value;

	inst->max_brightness = PDM_LED_DEFAULT_MAX_BRIGHTNESS;
	inst->brightness = 0;
	inst->enabled = 0;

	if (!np) {
		return 0;
	}

	if (!of_property_read_u32(np, "max-brightness", &value)) {
		if (!value) {
			return -EINVAL;
		}
		inst->max_brightness = value;
	}

	if (!of_property_read_u32(np, "default-brightness", &value)) {
		if (value > inst->max_brightness) {
			return -ERANGE;
		}
		inst->brightness = value;
	}

	if (!of_property_read_string(np, "default-state", &default_state)) {
		if (!strcmp(default_state, "on")) {
			inst->enabled = 1;
			if (!inst->brightness) {
				inst->brightness = inst->max_brightness;
			}
		} else if (!strcmp(default_state, "off")) {
			inst->enabled = 0;
		} else if (!strcmp(default_state, "keep")) {
			inst->enabled = inst->brightness ? 1 : 0;
		} else {
			return -EINVAL;
		}
	} else if (inst->brightness) {
		inst->enabled = 1;
	}

	return 0;
}

static long pdm_led_get_info(struct pdm_client *client, unsigned long arg)
{
	struct pdm_led_info info = {
		.abi_version = PDM_LED_ABI_VERSION,
		.module_version_major = 1,
		.module_version_minor = 0,
		.module_version_patch = 0,
		.open_count = pdm_client_open_count(client),
		.max_devices = (u32)atomic_read(&pdm_led_device_count),
	};

	if (copy_to_user((void __user *)arg, &info, sizeof(info))) {
		return -EFAULT;
	}
	return 0;
}

static long pdm_led_get_state(struct pdm_led_instance *inst, unsigned long arg)
{
	struct pdm_led_state state;
	int ret = 0;

	if (copy_from_user(&state, (void __user *)arg, sizeof(state))) {
		return -EFAULT;
	}

	mutex_lock(&inst->lock);
	if (!inst->online) {
		ret = -ENODEV;
		goto out_unlock;
	}

	state.brightness = inst->brightness;
	state.max_brightness = inst->max_brightness;
	state.enabled = inst->enabled;

out_unlock:
	mutex_unlock(&inst->lock);
	if (ret) {
		return ret;
	}

	if (copy_to_user((void __user *)arg, &state, sizeof(state))) {
		return -EFAULT;
	}
	return 0;
}

static int pdm_led_apply_locked(struct pdm_led_instance *inst)
{
	if (!inst->ops || !inst->ops->apply) {
		return -EOPNOTSUPP;
	}

	return inst->ops->apply(inst);
}

static long pdm_led_set_brightness(struct pdm_led_instance *inst,
				   unsigned long arg)
{
	struct pdm_led_brightness brightness;
	u32 old_brightness;
	u32 old_enabled;
	int ret;

	if (copy_from_user(&brightness, (void __user *)arg, sizeof(brightness))) {
		return -EFAULT;
	}
	if (brightness.brightness > inst->max_brightness) {
		return -ERANGE;
	}

	mutex_lock(&inst->lock);
	if (!inst->online) {
		ret = -ENODEV;
		goto out_unlock;
	}

	old_brightness = inst->brightness;
	old_enabled = inst->enabled;
	inst->brightness = brightness.brightness;
	if (brightness.brightness) {
		inst->enabled = 1;
	}

	ret = pdm_led_apply_locked(inst);
	if (ret) {
		inst->brightness = old_brightness;
		inst->enabled = old_enabled;
	}

out_unlock:
	mutex_unlock(&inst->lock);
	return ret;
}

static long pdm_led_set_enabled(struct pdm_led_instance *inst,
				unsigned long arg, bool enabled)
{
	u32 index;
	u32 old_enabled;
	int ret;

	if (copy_from_user(&index, (void __user *)arg, sizeof(index))) {
		return -EFAULT;
	}

	mutex_lock(&inst->lock);
	if (!inst->online) {
		ret = -ENODEV;
		goto out_unlock;
	}

	old_enabled = inst->enabled;
	inst->enabled = enabled ? 1U : 0U;
	ret = pdm_led_apply_locked(inst);
	if (ret) {
		inst->enabled = old_enabled;
	}

out_unlock:
	mutex_unlock(&inst->lock);
	return ret;
}

static long pdm_led_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct pdm_client *client = pdm_client_from_file(filp);
	struct pdm_led_instance *inst;

	if (!client) {
		return -ENODEV;
	}
	inst = container_of(client, struct pdm_led_instance, client);

	switch (cmd) {
	case PDM_LED_IOC_GET_INFO:
		return pdm_led_get_info(client, arg);
	case PDM_LED_IOC_GET_STATE:
		return pdm_led_get_state(inst, arg);
	case PDM_LED_IOC_SET_BRIGHTNESS:
		return pdm_led_set_brightness(inst, arg);
	case PDM_LED_IOC_ENABLE:
		return pdm_led_set_enabled(inst, arg, true);
	case PDM_LED_IOC_DISABLE:
		return pdm_led_set_enabled(inst, arg, false);
	default:
		return -ENOTTY;
	}
}

static const struct file_operations pdm_led_fops = {
	.owner = THIS_MODULE,
	.open = pdm_client_default_open,
	.release = pdm_client_default_release,
	.unlocked_ioctl = pdm_led_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pdm_led_ioctl,
#endif
};

static void pdm_led_client_release(struct pdm_client *client)
{
	struct pdm_led_instance *inst;

	if (!client) {
		return;
	}

	inst = container_of(client, struct pdm_led_instance, client);
	kfree(inst);
}

static const struct pdm_led_backend_ops pdm_led_memory_ops = {
	.type = PDM_LED_BACKEND_MEMORY,
	.name = "memory",
	.capability = PDM_CTL_DEVICE_CAP_NONE,
	.setup = pdm_led_memory_setup,
	.cleanup = pdm_led_memory_cleanup,
	.apply = pdm_led_memory_apply,
};

const struct pdm_led_backend_ops *pdm_led_backend_select(const char *compatible)
{
	const struct pdm_backend_entry *entry;

	if (pdm_led_is_generic_compatible(compatible)) {
		return &pdm_led_memory_ops;
	}

	entry = pdm_backend_find(PDM_CTL_DEVICE_TYPE_LED,
				 PDM_BACKEND_CLASS_CONTROL, compatible);
	if (entry) {
		return entry->ops;
	}

	return &pdm_led_memory_ops;
}

static int pdm_led_probe(struct pdm_device *pdm_dev)
{
	struct pdm_led_instance *inst;
	char nodename[32];
	int ret;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst) {
		return -ENOMEM;
	}

	inst->pdm_dev = pdm_dev;
	inst->ops = pdm_led_backend_select(pdm_dev->compatible);
	mutex_init(&inst->lock);
	inst->online = true;

	ret = pdm_led_read_dt_config(inst);
	if (ret) {
		goto err_free;
	}

	ret = inst->ops->setup(inst);
	if (ret) {
		goto err_free;
	}

	ret = pdm_led_apply_locked(inst);
	if (ret) {
		goto err_cleanup_backend;
	}

	snprintf(nodename, sizeof(nodename), "led%d", pdm_dev->id);
	ret = pdm_client_register(&inst->client, pdm_dev, "pdm-led",
				  nodename, &pdm_led_fops, pdm_led_client_release);
	if (ret) {
		goto err_cleanup_backend;
	}

	pdm_dev->capabilities |= inst->ops->capability;
	pdm_device_set_drvdata(pdm_dev, inst);
	atomic_inc(&pdm_led_device_count);
	LOG_INFO("Registered LED %s backend for %s",
		 inst->ops->name, dev_name(&pdm_dev->dev));
	return 0;

err_cleanup_backend:
	inst->ops->cleanup(inst);
err_free:
	kfree(inst);
	return ret;
}

static void pdm_led_remove(struct pdm_device *pdm_dev)
{
	struct pdm_led_instance *inst = pdm_device_get_drvdata(pdm_dev);

	if (!inst) {
		return;
	}

	atomic_dec_if_positive(&pdm_led_device_count);
	pdm_device_set_drvdata(pdm_dev, NULL);

	mutex_lock(&inst->lock);
	inst->online = false;
	if (inst->ops && inst->ops->cleanup) {
		inst->ops->cleanup(inst);
	}
	mutex_unlock(&inst->lock);

	pdm_client_unregister(&inst->client);
}

static const struct of_device_id pdm_led_of_match[] = {
	{ .compatible = "pdm,led" },
	{ .compatible = "pdm,led-gpio" },
	{ .compatible = "pdm,led-pwm" },
	{ .compatible = "vendor,pdm-led" },
	{ .compatible = "vendor,pdm-led-gpio" },
	{ .compatible = "vendor,pdm-led-pwm" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_led_of_match);

static struct pdm_driver pdm_led_driver = {
	.driver = {
		.name = "pdm-led",
		.of_match_table = pdm_led_of_match,
	},
	.device_type = PDM_CTL_DEVICE_TYPE_LED,
	.capabilities = PDM_CTL_DEVICE_CAP_USER_IOCTL,
	.match = pdm_led_match,
	.probe = pdm_led_probe,
	.remove = pdm_led_remove,
};

static int pdm_led_driver_init(void)
{
	return pdm_bus_register_driver(THIS_MODULE, &pdm_led_driver);
}

static void pdm_led_driver_exit(void)
{
	pdm_bus_unregister_driver(&pdm_led_driver);
}

pdm_driver_register(led, pdm_led_driver_init, pdm_led_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM LED bus driver");
