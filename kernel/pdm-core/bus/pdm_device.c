// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_device.c
 * @brief PDM device management for bus integration
 */

#include <linux/module.h>
#include <linux/slab.h>
#include "pdm/core/pdm_bus.h"
#include "pdm/core/pdm_device_new.h"
#include "osal/osal_log.h"

/**
 * @brief Release callback for PDM device
 */
static void pdm_device_release(struct device *dev)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	LOG_DEBUG("PDM-DEVICE", "Releasing device [%s]", dev_name(dev));
	kfree(pdm_dev);
}

/**
 * @brief Allocates a new PDM device
 */
struct pdm_device *pdm_device_alloc(unsigned int size)
{
	struct pdm_device *pdm_dev;

	pdm_dev = kzalloc(sizeof(*pdm_dev) + size, GFP_KERNEL);
	if (!pdm_dev)
		return NULL;

	device_initialize(&pdm_dev->dev);
	pdm_dev->dev.bus = &pdm_bus_type;
	pdm_dev->dev.release = pdm_device_release;

	return pdm_dev;
}
EXPORT_SYMBOL_GPL(pdm_device_alloc);

/**
 * @brief Registers a PDM device on the bus
 */
int pdm_device_register(struct pdm_device *pdm_dev, const char *name)
{
	int ret;

	if (!pdm_dev || !name)
		return -EINVAL;

	dev_set_name(&pdm_dev->dev, "%s", name);

	ret = device_register(&pdm_dev->dev);
	if (ret) {
		LOG_ERROR("PDM-DEVICE", "Failed to register device [%s], error %d",
			  name, ret);
		put_device(&pdm_dev->dev);
		return ret;
	}

	LOG_DEBUG("PDM-DEVICE", "Device [%s] registered", name);
	return 0;
}
EXPORT_SYMBOL_GPL(pdm_device_register);

/**
 * @brief Unregisters a PDM device
 */
void pdm_device_unregister(struct pdm_device *pdm_dev)
{
	if (!pdm_dev)
		return;

	LOG_DEBUG("PDM-DEVICE", "Unregistering device [%s]",
		  dev_name(&pdm_dev->dev));
	device_unregister(&pdm_dev->dev);
}
EXPORT_SYMBOL_GPL(pdm_device_unregister);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM Device Management");
