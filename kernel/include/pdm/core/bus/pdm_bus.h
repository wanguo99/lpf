// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_bus.h
 * @brief PDM Linux bus_type public interface
 */

#ifndef PDM_BUS_H
#define PDM_BUS_H

#include <linux/compiler.h>
#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/version.h>

#include "pdm/core/device/pdm_device.h"

#define drv_to_pdm_driver(__drv) \
	container_of(__drv, struct pdm_driver, driver)

#define pdm_driver_register(_name, _init, _exit) \
	static const struct pdm_driver_entry \
	__pdm_driver_entry_##_name __used \
	__section("pdm_driver_entries") __aligned(sizeof(void *)) = { \
		.name = #_name, \
		.init = _init, \
		.exit = _exit, \
	}

#define module_pdm_driver(__pdm_driver) \
	static int __init __pdm_driver##_init(void) \
	{ \
		return pdm_bus_register_driver(THIS_MODULE, &(__pdm_driver)); \
	} \
	module_init(__pdm_driver##_init); \
	static void __exit __pdm_driver##_exit(void) \
	{ \
		pdm_bus_unregister_driver(&(__pdm_driver)); \
	} \
	module_exit(__pdm_driver##_exit)

/**
 * struct pdm_driver - PDM driver wrapper around struct device_driver
 * @driver: Embedded Linux device driver. Set name and of_match_table here.
 * @of_match_table: Optional compatibility alias for driver.of_match_table.
 * @device_type: PDM_CTL_DEVICE_TYPE_* value for devices handled by this driver.
 * @capabilities: Capability flags added to devices handled by this driver.
 * @match: Optional service-owned authoritative match callback.
 * @probe: Called after the PDM bus matches a device to this driver.
 * @remove: Called before a bound PDM device is detached.
 */
struct pdm_driver {
	struct device_driver driver;
	const struct of_device_id *of_match_table;
	u32 device_type;
	u64 capabilities;

	bool (*match)(const struct pdm_device *dev);
	int (*probe)(struct pdm_device *dev);
	void (*remove)(struct pdm_device *dev);
};

struct pdm_driver_entry {
	const char *name;
	int (*init)(void);
	void (*exit)(void);
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
extern struct bus_type pdm_bus_type;
#else
extern const struct bus_type pdm_bus_type;
#endif

/**
 * pdm_bus_find_device_by_parent() - find a PDM device by parent device
 *
 * The returned device has an elevated reference. The caller must release it
 * with pdm_device_put().
 */
struct pdm_device *pdm_bus_find_device_by_parent(struct device *parent);

int pdm_bus_for_each_dev(void *data, int (*fn)(struct device *dev, void *data));

int pdm_bus_register_driver(struct module *owner, struct pdm_driver *driver);
void pdm_bus_unregister_driver(struct pdm_driver *driver);

int pdm_driver_entries_init(void);
void pdm_driver_entries_exit(void);

int pdm_bus_init(void);
void pdm_bus_exit(void);

#endif /* PDM_BUS_H */
