// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_device_new.h
 * @brief New PDM device structure for bus_type integration
 *
 * This defines PDM devices as proper Linux devices that integrate
 * with the standard driver model.
 */

#ifndef PDM_DEVICE_NEW_H
#define PDM_DEVICE_NEW_H

#include <linux/device.h>
#include "osal.h"

/**
 * @brief PDM device structure (new bus-based design)
 *
 * This structure represents a PDM device that is registered on the PDM bus.
 * It inherits from struct device for full Linux driver model integration.
 */
struct pdm_device {
	struct device dev;		/**< Linux device structure (must be first) */
	const char *compatible;		/**< Device Tree compatible string */
	void *config_data;		/**< Device-specific configuration */
	u64 capabilities;		/**< Device capability flags */
	int id;				/**< Device instance ID */
};

/**
 * @brief Converts device to pdm_device
 */
#define dev_to_pdm_device(__dev) \
	container_of(__dev, struct pdm_device, dev)

/**
 * @brief Converts pdm_device to device
 */
#define pdm_device_to_dev(__pdm_dev) \
	(&(__pdm_dev)->dev)

/**
 * @brief Gets driver data from a PDM device
 */
static inline void *pdm_device_get_drvdata(struct pdm_device *pdm_dev)
{
	return dev_get_drvdata(&pdm_dev->dev);
}

/**
 * @brief Sets driver data for a PDM device
 */
static inline void pdm_device_set_drvdata(struct pdm_device *pdm_dev, void *data)
{
	dev_set_drvdata(&pdm_dev->dev, data);
}

/**
 * @brief Gets a reference to a PDM device
 */
static inline struct pdm_device *pdm_device_get(struct pdm_device *pdm_dev)
{
	if (!pdm_dev || !get_device(&pdm_dev->dev))
		return NULL;
	return pdm_dev;
}

/**
 * @brief Puts a reference to a PDM device
 */
static inline void pdm_device_put(struct pdm_device *pdm_dev)
{
	if (pdm_dev)
		put_device(&pdm_dev->dev);
}

/**
 * @brief Allocates a new PDM device
 * @param size Size of private data to allocate with the device
 * @return Allocated PDM device or NULL on failure
 */
struct pdm_device *pdm_device_alloc(unsigned int size);

/**
 * @brief Registers a PDM device on the bus
 * @param pdm_dev PDM device to register
 * @param name Device name
 * @return 0 on success, negative error code on failure
 */
int pdm_device_register(struct pdm_device *pdm_dev, const char *name);

/**
 * @brief Unregisters a PDM device
 * @param pdm_dev PDM device to unregister
 */
void pdm_device_unregister(struct pdm_device *pdm_dev);

#endif /* PDM_DEVICE_NEW_H */
