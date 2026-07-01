// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_bus.c
 * @brief PDM Bus implementation using standard Linux bus_type
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/string.h>
#include <linux/version.h>

#include "pdm/diag/pdm_sysfs.h"
#include "pdm/bus/pdm_of_bus.h"
#include "pdm/bus/pdm_bus.h"
#include "pdm/bus/pdm_device.h"
#include "pdm/log/pdm_log.h"

static int pdm_bus_device_match_parent(struct device *dev, const void *data)
{
	struct device *parent = (struct device *)data;

	return dev->parent == parent ? 1 : 0;
}

static int pdm_bus_device_probe(struct device *dev)
{
	struct pdm_device *pdm_dev;
	struct pdm_driver *pdm_drv;
	int ret;

	if (!dev || !dev->driver) {
		return -EINVAL;
	}

	pdm_dev = dev_to_pdm_device(dev);
	pdm_drv = drv_to_pdm_driver(dev->driver);

	if (pdm_dev->owner == PDM_MANAGER_DEVICE_OWNER_USER) {
		LOG_INFO("Skipping kernel probe for user-owned device [%s]",
			 dev_name(dev));
		return -ENODEV;
	}

	if (!pdm_drv->probe) {
		return -ENODEV;
	}

	LOG_DEBUG("Probing device [%s] with driver [%s]",
		  dev_name(dev), pdm_drv->driver.name);
	ret = pdm_device_bind(pdm_dev, pdm_drv->device_type,
			      pdm_drv->capabilities);
	if (ret) {
		pdm_device_record_error(pdm_dev, ret);
		return ret;
	}

	ret = pdm_drv->probe(pdm_dev);
	if (ret) {
		pdm_device_record_error(pdm_dev, ret);
		pdm_device_unbind(pdm_dev);
		return ret;
	}

	pdm_device_set_state(pdm_dev, PDM_MANAGER_DEVICE_STATE_BOUND);
	pdm_dev->last_error = 0;
	return 0;
}

static void pdm_bus_device_remove(struct device *dev)
{
	struct pdm_device *pdm_dev;
	struct pdm_driver *pdm_drv;

	if (!dev || !dev->driver) {
		return;
	}

	pdm_dev = dev_to_pdm_device(dev);
	pdm_drv = drv_to_pdm_driver(dev->driver);

	LOG_DEBUG("Removing device [%s]", dev_name(dev));
	if (pdm_drv->remove) {
		pdm_drv->remove(pdm_dev);
	}
	pdm_device_unbind(pdm_dev);
}

static int pdm_bus_match_compatible(const struct pdm_device *pdm_dev,
				    const struct device_driver *drv)
{
	const struct of_device_id *match;

	if (!pdm_dev->compatible || !drv->of_match_table) {
		return 0;
	}

	for (match = drv->of_match_table; match->compatible[0]; match++) {
		if (strcmp(match->compatible, pdm_dev->compatible) == 0) {
			return 1;
		}
	}

	return 0;
}

static int pdm_bus_device_match_impl(struct device *dev,
				     const struct device_driver *drv)
{
	struct pdm_device *pdm_dev;
	struct pdm_driver *pdm_drv;

	if (!dev || !drv) {
		return 0;
	}

	pdm_dev = dev_to_pdm_device(dev);
	pdm_drv = drv_to_pdm_driver(drv);
	if (pdm_dev->owner == PDM_MANAGER_DEVICE_OWNER_USER) {
		return 0;
	}
	if (pdm_drv->match) {
		return pdm_drv->match(pdm_dev) ? 1 : 0;
	}

	if (dev->of_node && of_driver_match_device(dev, drv)) {
		return 1;
	}

	return pdm_bus_match_compatible(pdm_dev, drv);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)
static int pdm_bus_device_match(struct device *dev, struct device_driver *drv)
{
	return pdm_bus_device_match_impl(dev, drv);
}
#else
static int pdm_bus_device_match(struct device *dev,
				const struct device_driver *drv)
{
	return pdm_bus_device_match_impl(dev, drv);
}
#endif

static int pdm_bus_device_suspend(struct device *dev)
{
	struct pdm_device *pdm_dev;
	struct pdm_driver *pdm_drv;

	if (!dev || !dev->driver)
		return 0;

	pdm_dev = dev_to_pdm_device(dev);
	pdm_drv = drv_to_pdm_driver(dev->driver);

	if (pdm_drv->suspend) {
		LOG_DEBUG("Suspending device [%s]", dev_name(dev));
		return pdm_drv->suspend(pdm_dev);
	}

	return 0;
}

static int pdm_bus_device_resume(struct device *dev)
{
	struct pdm_device *pdm_dev;
	struct pdm_driver *pdm_drv;

	if (!dev || !dev->driver)
		return 0;

	pdm_dev = dev_to_pdm_device(dev);
	pdm_drv = drv_to_pdm_driver(dev->driver);

	if (pdm_drv->resume) {
		LOG_DEBUG("Resuming device [%s]", dev_name(dev));
		return pdm_drv->resume(pdm_dev);
	}

	return 0;
}

static const struct dev_pm_ops pdm_bus_pm_ops = {
	.suspend = pdm_bus_device_suspend,
	.resume = pdm_bus_device_resume,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
struct bus_type pdm_bus_type = {
#else
const struct bus_type pdm_bus_type = {
#endif
	.name       = "pdm",
	.probe      = pdm_bus_device_probe,
	.remove     = pdm_bus_device_remove,
	.match      = pdm_bus_device_match,
	.dev_groups = pdm_device_attr_groups,
	.pm         = &pdm_bus_pm_ops,
};
EXPORT_SYMBOL_GPL(pdm_bus_type);

struct pdm_device *pdm_bus_find_device_by_parent(struct device *parent)
{
	struct device *dev;

	dev = bus_find_device(&pdm_bus_type, NULL, parent,
			      pdm_bus_device_match_parent);
	if (!dev) {
		return NULL;
	}

	return dev_to_pdm_device(dev);
}
EXPORT_SYMBOL_GPL(pdm_bus_find_device_by_parent);

int pdm_bus_for_each_dev(void *data, int (*fn)(struct device *dev, void *data))
{
	return bus_for_each_dev(&pdm_bus_type, NULL, data, fn);
}
EXPORT_SYMBOL_GPL(pdm_bus_for_each_dev);

int pdm_bus_register_driver(struct module *owner, struct pdm_driver *driver)
{
	int ret;

	if (!driver || !driver->driver.name) {
		return -EINVAL;
	}

	if (driver->of_match_table && !driver->driver.of_match_table) {
		driver->driver.of_match_table = driver->of_match_table;
	}

	driver->driver.owner = owner;
	driver->driver.bus = &pdm_bus_type;

	ret = driver_register(&driver->driver);
	if (ret) {
		LOG_ERROR("Failed to register driver [%s], error %d",
			  driver->driver.name, ret);
		return ret;
	}

	LOG_DEBUG("Driver [%s] registered", driver->driver.name);
	return 0;
}
EXPORT_SYMBOL_GPL(pdm_bus_register_driver);

void pdm_bus_unregister_driver(struct pdm_driver *driver)
{
	if (!driver) {
		return;
	}

	driver_unregister(&driver->driver);
	LOG_DEBUG("Driver [%s] unregistered", driver->driver.name);
}
EXPORT_SYMBOL_GPL(pdm_bus_unregister_driver);

int pdm_bus_init(void)
{
	int ret;

	ret = bus_register(&pdm_bus_type);
	if (ret < 0) {
		LOG_ERROR("Failed to register bus, error %d", ret);
		return ret;
	}

	LOG_INFO("PDM bus initialized");

	/* Register OF (Device Tree) enumerator if CONFIG_OF is enabled */
#ifdef CONFIG_OF
	ret = pdm_of_bus_init();
	if (ret) {
		LOG_ERROR("Failed to register OF enumerator: %d", ret);
		bus_unregister(&pdm_bus_type);
		return ret;
	}
	LOG_INFO("PDM OF enumerator registered");
#endif

	return 0;
}

void pdm_bus_exit(void)
{
#ifdef CONFIG_OF
	pdm_of_bus_exit();
	LOG_INFO("PDM OF enumerator unregistered");
#endif

	bus_unregister(&pdm_bus_type);
	LOG_INFO("PDM bus unregistered");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PDM Team");
MODULE_DESCRIPTION("PDM Bus Module - Standard Linux Bus Implementation");
