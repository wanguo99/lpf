// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_COMPAT_SPI_H
#define PDM_COMPAT_SPI_H

#include "pdm/compat/pdm_compat_features.h"

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

typedef int (*pdm_compat_spi_probe_t)(struct spi_device *spi);
typedef void (*pdm_compat_spi_remove_t)(struct spi_device *spi);

struct pdm_compat_spi_driver {
	struct spi_driver driver;
	pdm_compat_spi_probe_t probe;
	pdm_compat_spi_remove_t remove;
};

static inline struct pdm_compat_spi_driver *
pdm_compat_spi_from_device(struct spi_device *spi)
{
	struct device_driver *dev_driver;
	struct spi_driver *spi_driver;

	if (!spi) {
		return NULL;
	}

	dev_driver = spi->dev.driver;
	if (!dev_driver) {
		return NULL;
	}

	spi_driver = container_of(dev_driver, struct spi_driver, driver);
	return container_of(spi_driver, struct pdm_compat_spi_driver, driver);
}

static inline int pdm_compat_spi_probe(struct spi_device *spi)
{
	struct pdm_compat_spi_driver *driver;

	driver = pdm_compat_spi_from_device(spi);
	if (!driver || !driver->probe) {
		return -ENODEV;
	}

	return driver->probe(spi);
}

static inline void pdm_compat_spi_remove_device(struct spi_device *spi)
{
	struct pdm_compat_spi_driver *driver;

	driver = pdm_compat_spi_from_device(spi);
	if (driver && driver->remove) {
		driver->remove(spi);
	}
}

static inline void pdm_compat_spi_remove_void(struct spi_device *spi)
{
	pdm_compat_spi_remove_device(spi);
}

static inline int pdm_compat_spi_remove_int(struct spi_device *spi)
{
	pdm_compat_spi_remove_device(spi);
	return 0;
}

static inline int
pdm_compat_spi_driver_register(struct pdm_compat_spi_driver *driver)
{
	if (!driver) {
		return -EINVAL;
	}

	driver->driver.probe = pdm_compat_spi_probe;
#if PDM_KERNEL_HAS_VOID_SPI_REMOVE
	driver->driver.remove = pdm_compat_spi_remove_void;
#else
	driver->driver.remove = pdm_compat_spi_remove_int;
#endif

	return spi_register_driver(&driver->driver);
}

static inline void
pdm_compat_spi_driver_unregister(struct pdm_compat_spi_driver *driver)
{
	if (driver) {
		spi_unregister_driver(&driver->driver);
	}
}

#endif /* PDM_COMPAT_SPI_H */
