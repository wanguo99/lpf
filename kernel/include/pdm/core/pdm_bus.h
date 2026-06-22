// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_bus.h
 * @brief PDM Bus public interface
 *
 * This defines the standard Linux bus_type interface for PDM.
 */

#ifndef PDM_BUS_H
#define PDM_BUS_H

#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/version.h>

#include "pdm_device.h"

/**
 * @struct pdm_device_id
 * @brief Device ID information for PDM devices
 *
 * Used for driver matching with compatible strings
 */
struct pdm_device_id {
	char compatible[64];		/**< Compatibility string for DT matching */
	kernel_ulong_t driver_data;	/**< Driver private data */
};

/**
 * @struct pdm_driver
 * @brief PDM driver structure
 *
 * This wraps the standard Linux device_driver with PDM-specific callbacks
 */
struct pdm_driver {
	struct device_driver driver;		/**< Linux device driver */
	const struct of_device_id *of_match_table; /**< Device Tree match table */

	int (*probe)(struct pdm_device *dev);	/**< Probe callback */
	void (*remove)(struct pdm_device *dev);	/**< Remove callback */
};

/**
 * @brief Converts device_driver to pdm_driver
 */
#define drv_to_pdm_driver(__drv) \
	container_of(__drv, struct pdm_driver, driver)

/**
 * @brief Finds a PDM device by its parent device
 * @param parent Parent device pointer
 * @return PDM device pointer or NULL
 */
struct pdm_device *pdm_bus_find_device_by_parent(struct device *parent);

/**
 * @brief Iterates over all devices on the PDM bus
 * @param data User data passed to callback
 * @param fn Callback function for each device
 * @return 0 on success, error code on failure
 */
int pdm_bus_for_each_dev(void *data, int (*fn)(struct device *dev, void *data));

/**
 * @brief Registers a PDM driver
 * @param owner Module owner
 * @param driver PDM driver structure
 * @return 0 on success, negative error code on failure
 */
int pdm_bus_register_driver(struct module *owner, struct pdm_driver *driver);

/**
 * @brief Unregisters a PDM driver
 * @param driver PDM driver structure
 */
void pdm_bus_unregister_driver(struct pdm_driver *driver);

/**
 * @brief Helper macro to register a PDM driver
 */
#define pdm_driver_register(driver) \
	pdm_bus_register_driver(THIS_MODULE, driver)

/**
 * @brief Initializes the PDM bus
 * @return 0 on success, negative error code on failure
 */
int pdm_bus_init(void);

/**
 * @brief Exits the PDM bus
 */
void pdm_bus_exit(void);

/**
 * @brief PDM bus type
 *
 * This is the standard Linux bus_type for PDM
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
extern struct bus_type pdm_bus_type;
#else
extern const struct bus_type pdm_bus_type;
#endif

/**
 * @brief Helper macro for module PDM driver registration
 *
 * Similar to module_platform_driver()
 */
#define module_pdm_driver(__pdm_driver) \
	module_driver(__pdm_driver, pdm_bus_register_driver, \
		      pdm_bus_unregister_driver)

#endif /* PDM_BUS_H */
