// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu.c
 * @brief PDM MCU bus driver with registered transport backends
 */

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "pdm_mcu_internal.h"

#include "pdm/core/driver/pdm_backend.h"
#include "pdm/core/bus/pdm_bus.h"
#include "pdm/pdm_ctl.h"
#include "osal.h"

#define PDM_MCU_BUS_DEVICE_NAME_LEN 96

static atomic_t pdm_mcu_device_count = ATOMIC_INIT(0);

static const char *pdm_mcu_default_compatible(enum pdm_mcu_backend_type type)
{
	switch (type) {
	case PDM_MCU_BACKEND_UART:
		return "pdm,mcu-uart";
	case PDM_MCU_BACKEND_I2C:
		return "pdm,mcu-i2c";
	case PDM_MCU_BACKEND_SPI:
		return "pdm,mcu-spi";
	case PDM_MCU_BACKEND_CAN:
		return "pdm,mcu-can";
	default:
		return NULL;
	}
}

static int pdm_mcu_bus_device_id(struct device_node *np)
{
	u32 value;

	if (np) {
		if (!of_property_read_u32(np, "pdm,id", &value)) {
			return (int)value;
		}
		if (!of_property_read_u32(np, "pdm,index", &value)) {
			return (int)value;
		}
	}

	return -1;
}

static bool pdm_mcu_has_transport_compatible(const char *compatible)
{
	return compatible &&
	       (strstarts(compatible, "pdm,mcu-") ||
		strstarts(compatible, "vendor,pdm-mcu-"));
}

static bool pdm_mcu_match(const struct pdm_device *pdm_dev)
{
	return pdm_dev && pdm_mcu_has_transport_compatible(pdm_dev->compatible);
}

static void pdm_mcu_update_state_locked(struct pdm_mcu_instance *inst, int ret)
{
	if (ret < 0) {
		inst->last_error = ret;
		inst->state = inst->base.online ? PDM_MCU_STATE_ERROR : PDM_MCU_STATE_OFFLINE;
		inst->base.pdm_dev->last_error = ret;
		inst->base.pdm_dev->error_count++;
		if (inst->base.online) {
			inst->base.pdm_dev->state = PDM_CTL_DEVICE_STATE_ERROR;
		}
		return;
	}

	inst->last_error = 0;
	inst->state = inst->base.online ? PDM_MCU_STATE_READY : PDM_MCU_STATE_OFFLINE;
	inst->base.pdm_dev->last_error = 0;
	if (inst->base.online) {
		inst->base.pdm_dev->state = PDM_CTL_DEVICE_STATE_BOUND;
	}
}

static int pdm_mcu_claim_device(struct pdm_mcu_instance *inst)
{
	int ret = 0;

	mutex_lock(&inst->base.lock);

	if (!inst->base.online) {
		ret = -ENODEV;
	} else if (!inst->ops) {
		ret = -EOPNOTSUPP;
	}
	if (ret) {
		pdm_mcu_update_state_locked(inst, ret);
		mutex_unlock(&inst->base.lock);
	}

	return ret;
}

static void pdm_mcu_release_device(struct pdm_mcu_instance *inst, int ret)
{
	pdm_mcu_update_state_locked(inst, ret);
	mutex_unlock(&inst->base.lock);
}

static long pdm_mcu_get_info(struct pdm_client *client, unsigned long arg)
{
	struct pdm_mcu_info info = {
		.abi_version = PDM_MCU_ABI_VERSION,
		.module_version_major = 1,
		.module_version_minor = 0,
		.module_version_patch = 0,
		.open_count = pdm_client_open_count(client),
		.max_devices = (u32)atomic_read(&pdm_mcu_device_count),
	};

	if (copy_to_user((void __user *)arg, &info, sizeof(info))) {
		return -EFAULT;
	}
	return 0;
}

static long pdm_mcu_get_version(struct pdm_mcu_instance *inst, unsigned long arg)
{
	struct pdm_mcu_version version;
	int ret;

	if (copy_from_user(&version, (void __user *)arg, sizeof(version))) {
		return -EFAULT;
	}

	ret = pdm_mcu_claim_device(inst);
	if (ret) {
		return ret;
	}
	ret = pdm_mcu_protocol_get_version(inst, &version);
	pdm_mcu_release_device(inst, ret);
	if (ret) {
		return ret;
	}

	if (copy_to_user((void __user *)arg, &version, sizeof(version))) {
		return -EFAULT;
	}
	return 0;
}

static long pdm_mcu_get_status(struct pdm_mcu_instance *inst, unsigned long arg)
{
	struct pdm_mcu_status status;
	int ret;

	if (copy_from_user(&status, (void __user *)arg, sizeof(status))) {
		return -EFAULT;
	}

	ret = pdm_mcu_claim_device(inst);
	if (ret) {
		return ret;
	}
	ret = pdm_mcu_protocol_get_status(inst, &status);
	pdm_mcu_release_device(inst, ret);
	if (ret) {
		return ret;
	}

	if (copy_to_user((void __user *)arg, &status, sizeof(status))) {
		return -EFAULT;
	}
	return 0;
}

static long pdm_mcu_reset(struct pdm_mcu_instance *inst, unsigned long arg)
{
	u32 index;
	int ret;

	if (copy_from_user(&index, (void __user *)arg, sizeof(index))) {
		return -EFAULT;
	}

	ret = pdm_mcu_claim_device(inst);
	if (ret) {
		return ret;
	}
	ret = pdm_mcu_protocol_reset(inst, index);
	pdm_mcu_release_device(inst, ret);
	return ret;
}

static long pdm_mcu_command_ioctl(struct pdm_mcu_instance *inst, unsigned long arg)
{
	struct pdm_mcu_command command;
	int ret;

	if (copy_from_user(&command, (void __user *)arg, sizeof(command))) {
		return -EFAULT;
	}

	ret = pdm_mcu_claim_device(inst);
	if (ret) {
		return ret;
	}
	ret = pdm_mcu_protocol_command(inst, &command);
	pdm_mcu_release_device(inst, ret);
	if (ret) {
		return ret;
	}

	if (copy_to_user((void __user *)arg, &command, sizeof(command))) {
		return -EFAULT;
	}
	return 0;
}

static long pdm_mcu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct pdm_client *client = pdm_client_from_file(filp);
	struct pdm_mcu_instance *inst;

	if (!client) {
		return -ENODEV;
	}
	inst = container_of(client, struct pdm_mcu_instance, base.client);

	switch (cmd) {
	case PDM_MCU_IOC_GET_INFO:
		return pdm_mcu_get_info(client, arg);
	case PDM_MCU_IOC_GET_VERSION:
		return pdm_mcu_get_version(inst, arg);
	case PDM_MCU_IOC_GET_STATUS:
		return pdm_mcu_get_status(inst, arg);
	case PDM_MCU_IOC_RESET:
		return pdm_mcu_reset(inst, arg);
	case PDM_MCU_IOC_COMMAND:
		return pdm_mcu_command_ioctl(inst, arg);
	default:
		return -ENOTTY;
	}
}

static const struct file_operations pdm_mcu_fops = {
	.owner = THIS_MODULE,
	.open = pdm_client_default_open,
	.release = pdm_client_default_release,
	.unlocked_ioctl = pdm_mcu_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pdm_mcu_ioctl,
#endif
};

static void pdm_mcu_client_release(struct pdm_client *client)
{
	struct pdm_mcu_instance *inst;

	if (!client) {
		return;
	}

	inst = container_of(client, struct pdm_mcu_instance, base.client);
	kfree(inst);
}

static int pdm_mcu_probe(struct pdm_device *pdm_dev)
{
	const struct pdm_backend_entry *entry;
	struct pdm_mcu_instance *inst;
	char nodename[32];
	int ret;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst) {
		return -ENOMEM;
	}

	pdm_driver_init(&inst->base, pdm_dev);

	/* Find transport backend for this device */
	entry = pdm_backend_find(PDM_MCU_DEVICE_TYPE,
				 PDM_BACKEND_CLASS_TRANSPORT,
				 pdm_dev->compatible);
	if (!entry) {
		LOG_ERROR("No MCU transport backend for compatible %s",
			  pdm_dev->compatible ? pdm_dev->compatible : "<none>");
		ret = -ENODEV;
		goto err_free;
	}
	inst->ops = entry->ops;
	inst->start_time = ktime_get();
	inst->state = PDM_MCU_STATE_READY;

	ret = inst->ops->setup(inst);
	if (ret) {
		goto err_free;
	}

	snprintf(nodename, sizeof(nodename), "mcu%d", pdm_dev->id);
	ret = pdm_client_register(&inst->base.client, pdm_dev, "pdm-mcu",
				  nodename, &pdm_mcu_fops, pdm_mcu_client_release);
	if (ret) {
		goto err_cleanup_transport;
	}

	pdm_dev->capabilities |= inst->ops->capability;
	pdm_device_set_drvdata(pdm_dev, inst);
	atomic_inc(&pdm_mcu_device_count);
	LOG_INFO("Registered MCU %s transport for %s",
		 inst->ops->name, dev_name(&pdm_dev->dev));
	return 0;

err_cleanup_transport:
	inst->ops->cleanup(inst);
err_free:
	kfree(inst);
	return ret;
}

static void pdm_mcu_remove(struct pdm_device *pdm_dev)
{
	struct pdm_mcu_instance *inst = pdm_device_get_drvdata(pdm_dev);

	if (!inst) {
		return;
	}

	atomic_dec_if_positive(&pdm_mcu_device_count);
	pdm_device_set_drvdata(pdm_dev, NULL);

	pdm_driver_shutdown(&inst->base);
	inst->state = PDM_MCU_STATE_OFFLINE;
	if (inst->ops && inst->ops->cleanup) {
		inst->ops->cleanup(inst);
	}
	mutex_unlock(&inst->base.lock);

	pdm_client_unregister(&inst->base.client);
}

int pdm_mcu_register_bus_device(struct device *parent,
				enum pdm_mcu_backend_type type,
				struct pdm_mcu_bus_device *bus_dev)
{
	struct device_node *np;
	struct pdm_device *pdm_dev;
	const char *compatible;
	char name[PDM_MCU_BUS_DEVICE_NAME_LEN];
	int ret;
	int id;

	if (!parent || !bus_dev) {
		return -EINVAL;
	}

	np = parent->of_node;
	compatible = pdm_mcu_default_compatible(type);
	if (np) {
		of_property_read_string(np, "compatible", &compatible);
	}
	if (!compatible) {
		return -EINVAL;
	}

	pdm_dev = pdm_device_alloc(0);
	if (!pdm_dev) {
		return -ENOMEM;
	}

	id = pdm_mcu_bus_device_id(np);
	pdm_dev->dev.parent = parent;
	pdm_dev->dev.of_node = of_node_get(np);
	pdm_dev->compatible = compatible;
	pdm_dev->config_data = bus_dev;
	pdm_device_set_requested_id(pdm_dev, id);
	bus_dev->type = type;
	bus_dev->pdm_dev = pdm_dev;

	if (id >= 0) {
		snprintf(name, sizeof(name), "%s.pdm-mcu.%d",
			 dev_name(parent), id);
	} else {
		snprintf(name, sizeof(name), "%s.pdm-mcu", dev_name(parent));
	}
	ret = pdm_device_register(pdm_dev, name);
	if (ret) {
		bus_dev->pdm_dev = NULL;
		return ret;
	}

	LOG_INFO("Created bus-bound %s device %s", compatible, name);
	return 0;
}

void pdm_mcu_unregister_bus_device(struct pdm_mcu_bus_device *bus_dev)
{
	if (!bus_dev || !bus_dev->pdm_dev) {
		return;
	}

	pdm_device_unregister(bus_dev->pdm_dev);
	bus_dev->pdm_dev = NULL;
	bus_dev->inst = NULL;
}

static const struct of_device_id pdm_mcu_of_match[] = {
	{ .compatible = "pdm,mcu-uart" },
	{ .compatible = "pdm,mcu-can" },
	{ .compatible = "pdm,mcu-i2c" },
	{ .compatible = "pdm,mcu-spi" },
	{ .compatible = "vendor,pdm-mcu-uart" },
	{ .compatible = "vendor,pdm-mcu-can" },
	{ .compatible = "vendor,pdm-mcu-i2c" },
	{ .compatible = "vendor,pdm-mcu-spi" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_of_match);

static struct pdm_driver pdm_mcu_driver = {
	.driver = {
		.name = "pdm-mcu",
		.of_match_table = pdm_mcu_of_match,
	},
	.device_type = PDM_MCU_DEVICE_TYPE,
	.capabilities = PDM_MCU_CAP_USER_IOCTL,
	.match = pdm_mcu_match,
	.probe = pdm_mcu_probe,
	.remove = pdm_mcu_remove,
};

static int pdm_mcu_driver_init(void)
{
	return pdm_bus_register_driver(THIS_MODULE, &pdm_mcu_driver);
}

static void pdm_mcu_driver_exit(void)
{
	pdm_bus_unregister_driver(&pdm_mcu_driver);
}

pdm_driver_register(mcu, pdm_mcu_driver_init, pdm_mcu_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM MCU bus driver");
