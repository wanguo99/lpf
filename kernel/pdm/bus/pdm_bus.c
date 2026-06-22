// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_bus.c
 * @brief PDM Bus implementation using standard Linux bus_type
 *
 * This implements PDM as a proper Linux bus, replacing the pseudo-bus.
 * Based on the reference implementation from /home/wanguo/Github/pdm
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/version.h>

#include "pdm/core/pdm_bus.h"
#include "pdm/core/pdm_device_new.h"
#include "osal.h"

/* 使用新总线的设备结构，不再包含旧的 pdm_device.h */

/**
 * @brief Matches a device based on its parent device
 */
static int pdm_bus_device_match_parent(struct device *dev, const void *data)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);
	struct device *parent = (struct device *)data;
	return (pdm_dev->dev.parent == parent) ? 1 : 0;
}

/**
 * @brief Finds a device on the pdm_bus_type that matches the parent
 */
struct pdm_device *pdm_bus_find_device_by_parent(struct device *parent)
{
	struct device *dev;

	dev = bus_find_device(&pdm_bus_type, NULL, parent,
			      pdm_bus_device_match_parent);
	if (!dev)
		return NULL;

	return dev_to_pdm_device(dev);
}
EXPORT_SYMBOL_GPL(pdm_bus_find_device_by_parent);

/**
 * @brief Iterates over all devices on the PDM bus
 */
int pdm_bus_for_each_dev(void *data, int (*fn)(struct device *dev, void *data))
{
	return bus_for_each_dev(&pdm_bus_type, NULL, data, fn);
}
EXPORT_SYMBOL_GPL(pdm_bus_for_each_dev);

/**
 * @brief Registers a PDM driver with the kernel
 */
int pdm_bus_register_driver(struct module *owner, struct pdm_driver *driver)
{
	int ret;

	if (!driver)
		return -EINVAL;

	driver->driver.owner = owner;
	driver->driver.bus = &pdm_bus_type;

	ret = driver_register(&driver->driver);
	if (ret) {
		LOG_ERROR("PDM-BUS", "Failed to register driver [%s], error %d",
			  driver->driver.name, ret);
		return ret;
	}

	LOG_DEBUG("PDM-BUS", "Driver [%s] registered", driver->driver.name);
	return 0;
}
EXPORT_SYMBOL_GPL(pdm_bus_register_driver);

/**
 * @brief Unregisters a PDM driver
 */
void pdm_bus_unregister_driver(struct pdm_driver *driver)
{
	if (!driver)
		return;

	driver_unregister(&driver->driver);
	LOG_DEBUG("PDM-BUS", "Driver [%s] unregistered", driver->driver.name);
}
EXPORT_SYMBOL_GPL(pdm_bus_unregister_driver);

/**
 * @brief Probes a PDM device
 */
static int pdm_bus_device_probe(struct device *dev)
{
	struct pdm_device *pdm_dev;
	struct pdm_driver *pdm_drv;

	if (!dev)
		return -EINVAL;

	pdm_dev = dev_to_pdm_device(dev);
	pdm_drv = drv_to_pdm_driver(dev->driver);

	if (pdm_dev && pdm_drv && pdm_drv->probe) {
		LOG_DEBUG("PDM-BUS", "Probing device [%s] with driver [%s]",
			  dev_name(dev), pdm_drv->driver.name);
		return pdm_drv->probe(pdm_dev);
	}

	LOG_WARN("PDM-BUS", "Driver or device not found or probe missing");
	return -ENODEV;
}

/**
 * @brief Removes a PDM device
 */
static void pdm_bus_device_remove(struct device *dev)
{
	struct pdm_device *pdm_dev;
	struct pdm_driver *pdm_drv;

	if (!dev)
		return;

	pdm_dev = dev_to_pdm_device(dev);
	pdm_drv = drv_to_pdm_driver(dev->driver);

	if (pdm_dev && pdm_drv && pdm_drv->remove) {
		LOG_DEBUG("PDM-BUS", "Removing device [%s]", dev_name(dev));
		pdm_drv->remove(pdm_dev);
	}
}

/**
 * @brief Matches a PDM device with a driver
 *
 * Uses Device Tree of_driver_match_device() for matching.
 * This is the standard Linux way of matching devices to drivers.
 */
static int pdm_bus_device_match_impl(struct device *dev,
				     const struct device_driver *drv)
{
	/* Use OF style match with parent device */
	if (of_driver_match_device(dev->parent, drv))
		return 1;

	return 0;
}

/* Kernel version compatibility wrapper */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)
static int pdm_bus_device_match(struct device *dev, struct device_driver *drv)
{
	return pdm_bus_device_match_impl(dev, (const struct device_driver *)drv);
}
#else
static int pdm_bus_device_match(struct device *dev,
				const struct device_driver *drv)
{
	return pdm_bus_device_match_impl(dev, drv);
}
#endif

/**
 * @brief PDM bus type definition
 *
 * This is the standard Linux bus_type structure that integrates
 * PDM into the kernel's driver model.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
struct bus_type pdm_bus_type = {
#else
const struct bus_type pdm_bus_type = {
#endif
	.name   = "pdm",
	.probe  = pdm_bus_device_probe,
	.remove = pdm_bus_device_remove,
	.match  = pdm_bus_device_match,
};
EXPORT_SYMBOL_GPL(pdm_bus_type);

/**
 * @brief Initializes the PDM bus
 */
int pdm_bus_init(void)
{
	int ret;

	ret = bus_register(&pdm_bus_type);
	if (ret < 0) {
		LOG_ERROR("PDM-BUS", "Failed to register bus, error %d", ret);
		return ret;
	}

	LOG_INFO("PDM-BUS", "PDM bus initialized");
	return 0;
}

/**
 * @brief Exits the PDM bus
 */
void pdm_bus_exit(void)
{
	bus_unregister(&pdm_bus_type);
	LOG_INFO("PDM-BUS", "PDM bus unregistered");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PDM Team");
MODULE_DESCRIPTION("PDM Bus Module - Standard Linux Bus Implementation");
