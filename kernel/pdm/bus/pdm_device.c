// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_device.c
 * @brief PDM device management for bus integration
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/idr.h>
#include <linux/slab.h>

#include "pdm/core/pdm_bus.h"
#include "pdm/core/pdm_device.h"
#include "osal.h"

static DEFINE_IDA(pdm_device_mcu_ida);
static DEFINE_IDA(pdm_device_led_ida);

static struct ida *pdm_device_ida_for_type(u32 type)
{
	switch (type) {
	case PDM_CTL_DEVICE_TYPE_MCU:
		return &pdm_device_mcu_ida;
	case PDM_CTL_DEVICE_TYPE_LED:
		return &pdm_device_led_ida;
	default:
		return NULL;
	}
}

static void pdm_device_release(struct device *dev)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	LOG_DEBUG("PDM-DEVICE", "Releasing device [%s]", dev_name(dev));
	pdm_device_unbind(pdm_dev);
	of_node_put(dev->of_node);
	kfree(pdm_dev);
}

struct pdm_device *pdm_device_alloc(unsigned int size)
{
	struct pdm_device *pdm_dev;

	pdm_dev = kzalloc(sizeof(*pdm_dev) + size, GFP_KERNEL);
	if (!pdm_dev)
		return NULL;

	device_initialize(&pdm_dev->dev);
	pdm_dev->dev.bus = &pdm_bus_type;
	pdm_dev->dev.release = pdm_device_release;
	pdm_dev->type = PDM_CTL_DEVICE_TYPE_INVALID;
	pdm_dev->state = PDM_CTL_DEVICE_STATE_REGISTERED;
	pdm_dev->id = -1;
	pdm_dev->requested_id = -1;

	return pdm_dev;
}
EXPORT_SYMBOL_GPL(pdm_device_alloc);

int pdm_device_register(struct pdm_device *pdm_dev, const char *name)
{
	int ret;

	if (!pdm_dev || !name)
		return -EINVAL;

	ret = dev_set_name(&pdm_dev->dev, "%s", name);
	if (ret) {
		LOG_ERROR("PDM-DEVICE", "Failed to name device [%s], error %d",
			  name, ret);
		put_device(&pdm_dev->dev);
		return ret;
	}

	ret = device_add(&pdm_dev->dev);
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

void pdm_device_unregister(struct pdm_device *pdm_dev)
{
	if (!pdm_dev)
		return;

	LOG_DEBUG("PDM-DEVICE", "Unregistering device [%s]",
		  dev_name(&pdm_dev->dev));
	device_unregister(&pdm_dev->dev);
}
EXPORT_SYMBOL_GPL(pdm_device_unregister);

void pdm_device_set_requested_id(struct pdm_device *pdm_dev, int id)
{
	if (!pdm_dev)
		return;

	pdm_dev->requested_id = id;
	if (!pdm_dev->id_allocated)
		pdm_dev->id = id;
}
EXPORT_SYMBOL_GPL(pdm_device_set_requested_id);

int pdm_device_bind(struct pdm_device *pdm_dev, u32 type, u64 capabilities)
{
	struct ida *ida;
	int id;

	if (!pdm_dev)
		return -EINVAL;

	ida = pdm_device_ida_for_type(type);
	if (!ida)
		return -EINVAL;

	if (pdm_dev->id_allocated) {
		if (pdm_dev->type != type)
			return -EBUSY;
		pdm_dev->capabilities |= capabilities;
		return 0;
	}

	if (pdm_dev->requested_id >= 0)
		id = ida_alloc_range(ida, pdm_dev->requested_id, pdm_dev->requested_id,
				     GFP_KERNEL);
	else
		id = ida_alloc(ida, GFP_KERNEL);
	if (id < 0)
		return id;

	pdm_dev->id = id;
	pdm_dev->id_allocated = true;
	pdm_dev->type = type;
	pdm_dev->capabilities |= capabilities;
	pdm_device_set_state(pdm_dev, PDM_CTL_DEVICE_STATE_REGISTERED);
	return 0;
}
EXPORT_SYMBOL_GPL(pdm_device_bind);

void pdm_device_unbind(struct pdm_device *pdm_dev)
{
	struct ida *ida;

	if (!pdm_dev || !pdm_dev->id_allocated)
		return;

	ida = pdm_device_ida_for_type(pdm_dev->type);
	if (ida)
		ida_free(ida, pdm_dev->id);

	pdm_dev->id_allocated = false;
	pdm_dev->id = pdm_dev->requested_id;
	pdm_dev->type = PDM_CTL_DEVICE_TYPE_INVALID;
	pdm_dev->capabilities = PDM_CTL_DEVICE_CAP_NONE;
	pdm_device_set_state(pdm_dev, PDM_CTL_DEVICE_STATE_REGISTERED);
}
EXPORT_SYMBOL_GPL(pdm_device_unbind);

void pdm_device_ids_destroy(void)
{
	ida_destroy(&pdm_device_led_ida);
	ida_destroy(&pdm_device_mcu_ida);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM Device Management");
