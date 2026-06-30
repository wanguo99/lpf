// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_device.c
 * @brief PDM device management for bus integration
 */

#include <linux/idr.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "pdm/bus/pdm_bus.h"
#include "pdm/bus/pdm_device.h"
#include "pdm/pdm_manager.h"
#include "osal.h"

/* ========================================================================
 * Unified ID Management
 * ======================================================================== */

struct pdm_id_allocator {
	struct ida ida;
	const char *name;
	atomic_t count;
};

/* ID allocators indexed by device type */
static struct pdm_id_allocator pdm_id_allocators[] = {
	[PDM_MANAGER_DEVICE_TYPE_MCU] = {
		.name = "mcu",
		.count = ATOMIC_INIT(0),
	},
	[PDM_MANAGER_DEVICE_TYPE_LED] = {
		.name = "led",
		.count = ATOMIC_INIT(0),
	},
};

#define PDM_ID_ALLOCATORS_SIZE ARRAY_SIZE(pdm_id_allocators)

static struct pdm_id_allocator *pdm_id_get_allocator(u32 device_type)
{
	if (device_type >= PDM_ID_ALLOCATORS_SIZE)
		return NULL;

	if (!pdm_id_allocators[device_type].name)
		return NULL;

	return &pdm_id_allocators[device_type];
}

static int pdm_id_alloc(u32 device_type, int requested_id)
{
	struct pdm_id_allocator *allocator;
	int id;

	allocator = pdm_id_get_allocator(device_type);
	if (!allocator) {
		LOG_ERROR("Invalid device type: 0x%x", device_type);
		return -EINVAL;
	}

	if (requested_id >= 0) {
		id = ida_alloc_range(&allocator->ida, requested_id,
				     requested_id, GFP_KERNEL);
	} else {
		id = ida_alloc(&allocator->ida, GFP_KERNEL);
	}

	if (id >= 0) {
		atomic_inc(&allocator->count);
		LOG_DEBUG("Allocated %s ID %d", allocator->name, id);
	} else {
		LOG_ERROR("Failed to allocate %s ID: %d",
			  allocator->name, id);
	}

	return id;
}

static void pdm_id_free(u32 device_type, int id)
{
	struct pdm_id_allocator *allocator;

	if (id < 0)
		return;

	allocator = pdm_id_get_allocator(device_type);
	if (!allocator)
		return;

	ida_free(&allocator->ida, id);
	atomic_dec_if_positive(&allocator->count);
	LOG_DEBUG("Freed %s ID %d", allocator->name, id);
}

static int pdm_id_init(void)
{
	size_t i;

	for (i = 0; i < PDM_ID_ALLOCATORS_SIZE; i++) {
		if (pdm_id_allocators[i].name)
			ida_init(&pdm_id_allocators[i].ida);
	}

	LOG_INFO("PDM ID management initialized");
	return 0;
}

static void pdm_id_destroy(void)
{
	size_t i;

	for (i = 0; i < PDM_ID_ALLOCATORS_SIZE; i++) {
		if (pdm_id_allocators[i].name)
			ida_destroy(&pdm_id_allocators[i].ida);
	}

	LOG_INFO("PDM ID management destroyed");
}

struct pdm_device_compatible_info {
	const char *compatible;
	u32 type;
	u32 transport;
	u64 capability;
};

static const struct pdm_device_compatible_info pdm_device_compatible_table[] = {
	{ "pdm,mcu-can", PDM_MANAGER_DEVICE_TYPE_MCU,
	  PDM_MANAGER_TRANSPORT_CAN, PDM_MANAGER_DEVICE_CAP_TRANSPORT_CAN },
	{ "vendor,pdm-mcu-can", PDM_MANAGER_DEVICE_TYPE_MCU,
	  PDM_MANAGER_TRANSPORT_CAN, PDM_MANAGER_DEVICE_CAP_TRANSPORT_CAN },
	{ "pdm,mcu-uart", PDM_MANAGER_DEVICE_TYPE_MCU,
	  PDM_MANAGER_TRANSPORT_UART, PDM_MANAGER_DEVICE_CAP_TRANSPORT_UART },
	{ "vendor,pdm-mcu-uart", PDM_MANAGER_DEVICE_TYPE_MCU,
	  PDM_MANAGER_TRANSPORT_UART, PDM_MANAGER_DEVICE_CAP_TRANSPORT_UART },
	{ "pdm,mcu-i2c", PDM_MANAGER_DEVICE_TYPE_MCU,
	  PDM_MANAGER_TRANSPORT_I2C, PDM_MANAGER_DEVICE_CAP_TRANSPORT_I2C },
	{ "vendor,pdm-mcu-i2c", PDM_MANAGER_DEVICE_TYPE_MCU,
	  PDM_MANAGER_TRANSPORT_I2C, PDM_MANAGER_DEVICE_CAP_TRANSPORT_I2C },
	{ "pdm,mcu-spi", PDM_MANAGER_DEVICE_TYPE_MCU,
	  PDM_MANAGER_TRANSPORT_SPI, PDM_MANAGER_DEVICE_CAP_TRANSPORT_SPI },
	{ "vendor,pdm-mcu-spi", PDM_MANAGER_DEVICE_TYPE_MCU,
	  PDM_MANAGER_TRANSPORT_SPI, PDM_MANAGER_DEVICE_CAP_TRANSPORT_SPI },
};

static const struct pdm_device_compatible_info *
pdm_device_lookup_compatible(const char *compatible)
{
	size_t i;

	if (!compatible)
		return NULL;

	for (i = 0; i < ARRAY_SIZE(pdm_device_compatible_table); i++) {
		if (!strcmp(compatible, pdm_device_compatible_table[i].compatible))
			return &pdm_device_compatible_table[i];
	}

	return NULL;
}

u32 pdm_device_of_owner(struct device_node *np)
{
	const char *owner;

	if (!np || of_property_read_string(np, "pdm,owner", &owner))
		return PDM_MANAGER_DEVICE_OWNER_KERNEL;

	if (!strcmp(owner, "kernel"))
		return PDM_MANAGER_DEVICE_OWNER_KERNEL;
	if (!strcmp(owner, "user"))
		return PDM_MANAGER_DEVICE_OWNER_USER;

	LOG_WARN("Invalid pdm,owner value %s, defaulting to kernel", owner);
	return PDM_MANAGER_DEVICE_OWNER_KERNEL;
}
EXPORT_SYMBOL_GPL(pdm_device_of_owner);

static struct device_node *pdm_device_controller_from_of(struct device_node *np)
{
	struct device_node *controller;

	if (!np)
		return NULL;

	controller = of_parse_phandle(np, "can-controller", 0);
	if (controller)
		return controller;
	controller = of_parse_phandle(np, "can", 0);
	if (controller)
		return controller;
	controller = of_parse_phandle(np, "serial-controller", 0);
	if (controller)
		return controller;
	controller = of_parse_phandle(np, "uart-controller", 0);
	if (controller)
		return controller;
	controller = of_parse_phandle(np, "i2c-controller", 0);
	if (controller)
		return controller;
	controller = of_parse_phandle(np, "spi-controller", 0);
	if (controller)
		return controller;

	return of_node_get(np->parent);
}

int pdm_device_of_alias_id(struct device_node *np, const char *stem)
{
	int id;

	if (!np || !stem)
		return -ENODEV;

	id = of_alias_get_id(np, stem);
	return id >= 0 ? id : -ENODEV;
}
EXPORT_SYMBOL_GPL(pdm_device_of_alias_id);

void pdm_device_apply_of_metadata(struct pdm_device *pdm_dev)
{
	const struct pdm_device_compatible_info *info;
	struct device_node *controller;

	if (!pdm_dev)
		return;

	info = pdm_device_lookup_compatible(pdm_dev->compatible);
	pdm_dev->transport = info ? info->transport : PDM_MANAGER_TRANSPORT_NONE;
	pdm_dev->owner = pdm_device_of_owner(pdm_dev->dev.of_node);

	controller = pdm_device_controller_from_of(pdm_dev->dev.of_node);
	if (controller != pdm_dev->controller_node) {
		of_node_put(pdm_dev->controller_node);
		pdm_dev->controller_node = controller;
	} else if (controller) {
		of_node_put(controller);
	}
}
EXPORT_SYMBOL_GPL(pdm_device_apply_of_metadata);

/* ========================================================================
 * PDM Device Management
 * ======================================================================== */

static void pdm_device_release(struct device *dev)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	LOG_DEBUG("Releasing device [%s]", dev_name(dev));
	pdm_device_unbind(pdm_dev);
	of_node_put(pdm_dev->controller_node);
	pdm_dev->controller_node = NULL;
	of_node_put(dev->of_node);
	kfree(pdm_dev);
}

struct pdm_device *pdm_device_alloc(unsigned int size)
{
	struct pdm_device *pdm_dev;

	pdm_dev = kzalloc(sizeof(*pdm_dev) + size, GFP_KERNEL);
	if (!pdm_dev) {
		return NULL;
	}

	device_initialize(&pdm_dev->dev);
	pdm_dev->dev.bus = &pdm_bus_type;
	pdm_dev->dev.release = pdm_device_release;
	pdm_dev->type = PDM_MANAGER_DEVICE_TYPE_INVALID;
	pdm_dev->state = PDM_MANAGER_DEVICE_STATE_REGISTERED;
	pdm_dev->owner = PDM_MANAGER_DEVICE_OWNER_KERNEL;
	pdm_dev->transport = PDM_MANAGER_TRANSPORT_NONE;
	pdm_dev->id = -1;
	pdm_dev->requested_id = -1;

	return pdm_dev;
}
EXPORT_SYMBOL_GPL(pdm_device_alloc);

int pdm_device_register(struct pdm_device *pdm_dev, const char *name)
{
	bool reserved_user = false;
	int ret;

	if (!pdm_dev || !name) {
		return -EINVAL;
	}

	ret = dev_set_name(&pdm_dev->dev, "%s", name);
	if (ret) {
		LOG_ERROR("Failed to name device [%s], error %d",
			  name, ret);
		put_device(&pdm_dev->dev);
		return ret;
	}

	if (pdm_dev->owner == PDM_MANAGER_DEVICE_OWNER_USER) {
		const struct pdm_device_compatible_info *info;

		info = pdm_device_lookup_compatible(pdm_dev->compatible);
		if (!info || info->type == PDM_MANAGER_DEVICE_TYPE_INVALID ||
		    info->transport == PDM_MANAGER_TRANSPORT_NONE) {
			LOG_ERROR("Cannot reserve user-owned device [%s] with compatible %s",
				  name, pdm_dev->compatible ? pdm_dev->compatible : "<none>");
			put_device(&pdm_dev->dev);
			return -EINVAL;
		}

		ret = pdm_device_bind(pdm_dev, info->type,
				      info->capability);
		if (ret) {
			LOG_ERROR("Failed to reserve user-owned device [%s], error %d",
				  name, ret);
			put_device(&pdm_dev->dev);
			return ret;
		}
		reserved_user = true;
	}

	ret = device_add(&pdm_dev->dev);
	if (ret) {
		LOG_ERROR("Failed to register device [%s], error %d",
			  name, ret);
		if (reserved_user)
			pdm_device_unbind(pdm_dev);
		put_device(&pdm_dev->dev);
		return ret;
	}

	LOG_DEBUG("Device [%s] registered", name);
	return 0;
}
EXPORT_SYMBOL_GPL(pdm_device_register);

void pdm_device_unregister(struct pdm_device *pdm_dev)
{
	if (!pdm_dev) {
		return;
	}

	LOG_DEBUG("Unregistering device [%s]",
		  dev_name(&pdm_dev->dev));
	device_unregister(&pdm_dev->dev);
}
EXPORT_SYMBOL_GPL(pdm_device_unregister);

void pdm_device_set_requested_id(struct pdm_device *pdm_dev, int id)
{
	if (!pdm_dev) {
		return;
	}

	pdm_dev->requested_id = id;
	if (!pdm_dev->id_allocated) {
		pdm_dev->id = id;
	}
}
EXPORT_SYMBOL_GPL(pdm_device_set_requested_id);

int pdm_device_bind(struct pdm_device *pdm_dev, u32 type, u64 capabilities)
{
	int id;

	if (!pdm_dev)
		return -EINVAL;

	if (pdm_dev->id_allocated) {
		if (pdm_dev->type != type)
			return -EBUSY;
		pdm_dev->capabilities |= capabilities;
		return 0;
	}

	id = pdm_id_alloc(type, pdm_dev->requested_id);
	if (id < 0)
		return id;

	pdm_dev->id = id;
	pdm_dev->id_allocated = true;
	pdm_dev->type = type;
	pdm_dev->capabilities |= capabilities;
	pdm_device_set_state(pdm_dev, PDM_MANAGER_DEVICE_STATE_REGISTERED);
	return 0;
}
EXPORT_SYMBOL_GPL(pdm_device_bind);

void pdm_device_unbind(struct pdm_device *pdm_dev)
{
	if (!pdm_dev || !pdm_dev->id_allocated)
		return;

	pdm_id_free(pdm_dev->type, pdm_dev->id);

	pdm_dev->id_allocated = false;
	pdm_dev->id = pdm_dev->requested_id;
	pdm_dev->type = PDM_MANAGER_DEVICE_TYPE_INVALID;
	pdm_dev->capabilities = PDM_MANAGER_DEVICE_CAP_NONE;
	pdm_device_set_state(pdm_dev, PDM_MANAGER_DEVICE_STATE_REGISTERED);
}
EXPORT_SYMBOL_GPL(pdm_device_unbind);

void pdm_device_ids_destroy(void)
{
	pdm_id_destroy();
}

int pdm_device_ids_init(void)
{
	return pdm_id_init();
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM Device Management");

