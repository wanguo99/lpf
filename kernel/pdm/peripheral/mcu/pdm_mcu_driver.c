// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_driver.c
 * @brief PDM MCU bus driver with generic, UART and CAN transports
 */

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "../pdm_peripheral.h"
#include "pdm_mcu_internal.h"

#include "pdm/core/pdm_bus.h"
#include "pdm/pdm_ctl.h"
#include "osal.h"

static atomic_t pdm_mcu_device_count = ATOMIC_INIT(0);
static atomic_t pdm_mcu_native_auto_id = ATOMIC_INIT(0);

#define PDM_MCU_NATIVE_DEVICE_NAME_LEN 96

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
		return "pdm,mcu";
	}
}

static int pdm_mcu_native_device_id(struct device_node *np)
{
	u32 value;

	if (np) {
		if (!of_property_read_u32(np, "pdm,id", &value))
			return (int)value;
		if (!of_property_read_u32(np, "pdm,index", &value))
			return (int)value;
		if (!of_property_read_u32(np, "reg", &value))
			return (int)value;
	}

	return atomic_inc_return(&pdm_mcu_native_auto_id) - 1;
}

int pdm_mcu_register_native_device(struct device *parent,
				   enum pdm_mcu_backend_type type,
				   struct pdm_mcu_native_device *native)
{
	struct device_node *np;
	struct pdm_device *pdm_dev;
	const char *compatible;
	char name[PDM_MCU_NATIVE_DEVICE_NAME_LEN];
	int ret;
	int id;

	if (!parent || !native)
		return -EINVAL;

	np = parent->of_node;
	compatible = pdm_mcu_default_compatible(type);
	if (np)
		of_property_read_string(np, "compatible", &compatible);
	if (!compatible)
		return -EINVAL;

	pdm_dev = pdm_device_alloc(0);
	if (!pdm_dev)
		return -ENOMEM;

	id = pdm_mcu_native_device_id(np);
	pdm_dev->dev.parent = parent;
	pdm_dev->dev.of_node = of_node_get(np);
	pdm_dev->compatible = compatible;
	pdm_dev->config_data = native;
	pdm_dev->id = id;
	native->type = type;
	native->pdm_dev = pdm_dev;

	snprintf(name, sizeof(name), "%s.pdm-mcu.%d", dev_name(parent), id);
	ret = pdm_device_register(pdm_dev, name);
	if (ret) {
		native->pdm_dev = NULL;
		return ret;
	}

	LOG_INFO("PDM-MCU", "Created native %s device %s",
		 pdm_mcu_default_compatible(type), name);
	return 0;
}

void pdm_mcu_unregister_native_device(struct pdm_mcu_native_device *native)
{
	if (!native || !native->pdm_dev)
		return;

	pdm_device_unregister(native->pdm_dev);
	native->pdm_dev = NULL;
	native->inst = NULL;
}

static int pdm_mcu_memory_setup(struct pdm_mcu_instance *inst)
{
	(void)inst;
	return 0;
}

static void pdm_mcu_memory_cleanup(struct pdm_mcu_instance *inst)
{
	(void)inst;
}

static int pdm_mcu_memory_reset(struct pdm_mcu_instance *inst)
{
	(void)inst;
	return 0;
}

static int pdm_mcu_memory_command(struct pdm_mcu_instance *inst,
				  struct pdm_mcu_command *command)
{
	(void)inst;
	(void)command;
	return -EOPNOTSUPP;
}

static int pdm_mcu_memory_read_data(struct pdm_mcu_instance *inst,
				    struct pdm_mcu_data *data)
{
	(void)inst;
	(void)data;
	return -EOPNOTSUPP;
}

static int pdm_mcu_memory_write_data(struct pdm_mcu_instance *inst,
				     const struct pdm_mcu_data *data)
{
	(void)inst;
	(void)data;
	return -EOPNOTSUPP;
}

static const struct pdm_mcu_transport_ops pdm_mcu_memory_ops = {
	.type = PDM_MCU_BACKEND_MEMORY,
	.name = "memory",
	.capability = PDM_CTL_DEVICE_CAP_NONE,
	.setup = pdm_mcu_memory_setup,
	.cleanup = pdm_mcu_memory_cleanup,
	.reset = pdm_mcu_memory_reset,
	.command = pdm_mcu_memory_command,
	.read_data = pdm_mcu_memory_read_data,
	.write_data = pdm_mcu_memory_write_data,
};

const struct pdm_mcu_transport_ops *pdm_mcu_transport_select(const char *compatible)
{
	if (!compatible)
		return &pdm_mcu_memory_ops;

	if (!strcmp(compatible, "pdm,mcu-uart") ||
	    !strcmp(compatible, "vendor,pdm-mcu-uart"))
		return &pdm_mcu_uart_ops;

	if (!strcmp(compatible, "pdm,mcu-can") ||
	    !strcmp(compatible, "vendor,pdm-mcu-can"))
		return &pdm_mcu_can_ops;

#if IS_ENABLED(CONFIG_PDM_MCU_I2C) && IS_ENABLED(CONFIG_I2C)
	if (!strcmp(compatible, "pdm,mcu-i2c") ||
	    !strcmp(compatible, "vendor,pdm-mcu-i2c"))
		return &pdm_mcu_i2c_ops;
#endif

#if IS_ENABLED(CONFIG_PDM_MCU_SPI) && IS_ENABLED(CONFIG_SPI)
	if (!strcmp(compatible, "pdm,mcu-spi") ||
	    !strcmp(compatible, "vendor,pdm-mcu-spi"))
		return &pdm_mcu_spi_ops;
#endif

	return &pdm_mcu_memory_ops;
}

static int pdm_mcu_record_result(struct pdm_mcu_instance *inst, int ret)
{
	if (ret < 0) {
		inst->last_error = ret;
		inst->state = inst->online ? PDM_MCU_STATE_ERROR : PDM_MCU_STATE_OFFLINE;
		inst->pdm_dev->last_error = ret;
		inst->pdm_dev->error_count++;
		if (inst->online)
			inst->pdm_dev->state = PDM_CTL_DEVICE_STATE_ERROR;
		return ret;
	}

	inst->last_error = 0;
	inst->state = inst->online ? PDM_MCU_STATE_READY : PDM_MCU_STATE_OFFLINE;
	inst->pdm_dev->last_error = 0;
	if (inst->online)
		inst->pdm_dev->state = PDM_CTL_DEVICE_STATE_BOUND;
	return ret;
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

	if (copy_to_user((void __user *)arg, &info, sizeof(info)))
		return -EFAULT;
	return 0;
}

static long pdm_mcu_get_version(struct pdm_mcu_instance *inst, unsigned long arg)
{
	struct pdm_mcu_version version;

	if (copy_from_user(&version, (void __user *)arg, sizeof(version)))
		return -EFAULT;

	version.major = 1;
	version.minor = 0;
	version.patch = 0;
	version.build = 0;
	snprintf(version.version_string, sizeof(version.version_string),
		 "pdm-mcu-%s", inst->ops ? inst->ops->name : "unknown");

	if (copy_to_user((void __user *)arg, &version, sizeof(version)))
		return -EFAULT;
	return 0;
}

static long pdm_mcu_get_status(struct pdm_mcu_instance *inst, unsigned long arg)
{
	struct pdm_mcu_status status;
	u64 elapsed_ns;
	int ret = 0;

	if (copy_from_user(&status, (void __user *)arg, sizeof(status)))
		return -EFAULT;

	mutex_lock(&inst->lock);
	elapsed_ns = ktime_to_ns(ktime_sub(ktime_get(), inst->start_time));
	status.online = inst->online ? 1U : 0U;
	status.state = inst->state;
	status.uptime_sec = (u32)(elapsed_ns / NSEC_PER_SEC);
	status.error_code = inst->last_error < 0 ? (u32)(-inst->last_error) : 0U;
	status.temperature_milli_celsius = 0;
	status.voltage_mv = 0;
	status.timestamp_us = ktime_to_us(ktime_get());
	if (!inst->online)
		ret = -ENODEV;
	mutex_unlock(&inst->lock);

	if (copy_to_user((void __user *)arg, &status, sizeof(status)))
		return -EFAULT;
	return ret;
}

static int pdm_mcu_require_online_locked(struct pdm_mcu_instance *inst)
{
	if (!inst->online)
		return -ENODEV;
	if (!inst->ops)
		return -EOPNOTSUPP;

	return 0;
}

static long pdm_mcu_reset(struct pdm_mcu_instance *inst, unsigned long arg)
{
	u32 index;
	int ret;

	if (copy_from_user(&index, (void __user *)arg, sizeof(index)))
		return -EFAULT;

	mutex_lock(&inst->lock);
	ret = pdm_mcu_require_online_locked(inst);
	if (!ret)
		ret = inst->ops->reset(inst);
	ret = pdm_mcu_record_result(inst, ret);
	mutex_unlock(&inst->lock);
	return ret;
}

static long pdm_mcu_command_ioctl(struct pdm_mcu_instance *inst, unsigned long arg)
{
	struct pdm_mcu_command command;
	int ret;

	if (copy_from_user(&command, (void __user *)arg, sizeof(command)))
		return -EFAULT;

	mutex_lock(&inst->lock);
	ret = pdm_mcu_require_online_locked(inst);
	if (!ret)
		ret = inst->ops->command(inst, &command);
	ret = pdm_mcu_record_result(inst, ret);
	mutex_unlock(&inst->lock);
	if (ret)
		return ret;

	if (copy_to_user((void __user *)arg, &command, sizeof(command)))
		return -EFAULT;
	return 0;
}

static long pdm_mcu_read_data_ioctl(struct pdm_mcu_instance *inst, unsigned long arg)
{
	struct pdm_mcu_data data;
	int ret;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
		return -EFAULT;

	mutex_lock(&inst->lock);
	ret = pdm_mcu_require_online_locked(inst);
	if (!ret)
		ret = inst->ops->read_data(inst, &data);
	ret = pdm_mcu_record_result(inst, ret);
	mutex_unlock(&inst->lock);
	if (ret)
		return ret;

	if (copy_to_user((void __user *)arg, &data, sizeof(data)))
		return -EFAULT;
	return 0;
}

static long pdm_mcu_write_data_ioctl(struct pdm_mcu_instance *inst, unsigned long arg)
{
	struct pdm_mcu_data data;
	int ret;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
		return -EFAULT;

	mutex_lock(&inst->lock);
	ret = pdm_mcu_require_online_locked(inst);
	if (!ret)
		ret = inst->ops->write_data(inst, &data);
	ret = pdm_mcu_record_result(inst, ret);
	mutex_unlock(&inst->lock);
	return ret;
}

static long pdm_mcu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct pdm_client *client = pdm_client_from_file(filp);
	struct pdm_mcu_instance *inst;

	if (!client)
		return -ENODEV;
	inst = container_of(client, struct pdm_mcu_instance, client);

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
	case PDM_MCU_IOC_READ_DATA:
		return pdm_mcu_read_data_ioctl(inst, arg);
	case PDM_MCU_IOC_WRITE_DATA:
		return pdm_mcu_write_data_ioctl(inst, arg);
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

	if (!client)
		return;

	inst = container_of(client, struct pdm_mcu_instance, client);
	kfree(inst);
}

static int pdm_mcu_probe(struct pdm_device *pdm_dev)
{
	struct pdm_mcu_instance *inst;
	char nodename[32];
	int ret;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;

	inst->pdm_dev = pdm_dev;
	inst->ops = pdm_mcu_transport_select(pdm_dev->compatible);
	inst->start_time = ktime_get();
	inst->online = true;
	inst->state = PDM_MCU_STATE_READY;
	mutex_init(&inst->lock);

	ret = inst->ops->setup(inst);
	if (ret)
		goto err_free;

	snprintf(nodename, sizeof(nodename), "mcu%d", pdm_dev->id);
	ret = pdm_client_register(&inst->client, pdm_dev, "pdm-mcu",
				  nodename, &pdm_mcu_fops, pdm_mcu_client_release);
	if (ret)
		goto err_cleanup_transport;

	pdm_dev->capabilities |= inst->ops->capability;
	pdm_device_set_drvdata(pdm_dev, inst);
	atomic_inc(&pdm_mcu_device_count);
	LOG_INFO("PDM-MCU", "Registered MCU %s transport for %s",
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

	if (!inst)
		return;

	atomic_dec_if_positive(&pdm_mcu_device_count);
	pdm_device_set_drvdata(pdm_dev, NULL);

	mutex_lock(&inst->lock);
	inst->online = false;
	inst->state = PDM_MCU_STATE_OFFLINE;
	if (inst->ops && inst->ops->cleanup)
		inst->ops->cleanup(inst);
	mutex_unlock(&inst->lock);

	pdm_client_unregister(&inst->client);
}

static const struct of_device_id pdm_mcu_of_match[] = {
	{ .compatible = "pdm,mcu" },
	{ .compatible = "pdm,mcu-uart" },
	{ .compatible = "pdm,mcu-can" },
	{ .compatible = "pdm,mcu-i2c" },
	{ .compatible = "pdm,mcu-spi" },
	{ .compatible = "vendor,pdm-mcu" },
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
	.device_type = PDM_CTL_DEVICE_TYPE_MCU,
	.capabilities = PDM_CTL_DEVICE_CAP_USER_IOCTL,
	.probe = pdm_mcu_probe,
	.remove = pdm_mcu_remove,
};

int pdm_mcu_driver_init(void)
{
	int ret;

	ret = pdm_bus_register_driver(THIS_MODULE, &pdm_mcu_driver);
	if (ret)
		return ret;

	ret = pdm_mcu_serdev_driver_register();
	if (ret)
		goto err_unregister_pdm;

	ret = pdm_mcu_i2c_driver_register();
	if (ret)
		goto err_unregister_serdev;

	ret = pdm_mcu_spi_driver_register();
	if (ret)
		goto err_unregister_i2c;

	return 0;

err_unregister_i2c:
	pdm_mcu_i2c_driver_unregister();
err_unregister_serdev:
	pdm_mcu_serdev_driver_unregister();
err_unregister_pdm:
	pdm_bus_unregister_driver(&pdm_mcu_driver);
	return ret;
}

void pdm_mcu_driver_exit(void)
{
	pdm_mcu_spi_driver_unregister();
	pdm_mcu_i2c_driver_unregister();
	pdm_mcu_serdev_driver_unregister();
	pdm_bus_unregister_driver(&pdm_mcu_driver);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM MCU bus driver");
