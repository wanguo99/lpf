// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mock_devices.c
 * @brief Synthetic PDM bus devices for x86/module smoke testing
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "pdm_mock_devices.h"

#include "pdm/bus/pdm_bus.h"
#include "pdm/bus/pdm_device.h"
#include "osal.h"

struct pdm_mock_device_desc {
	const char *name;
	const char *compatible;
	int id;
};

struct pdm_mock_device_entry {
	struct pdm_device *pdm_dev;
	struct device parent;
	const struct pdm_mock_device_desc *desc;
	bool parent_registered;
};

static const struct pdm_mock_device_desc pdm_mock_device_descs[] = {
	{ .name = "led", .compatible = "pdm,led", .id = 0 },
};

static struct pdm_mock_device_entry pdm_mock_devices[ARRAY_SIZE(pdm_mock_device_descs)];

static void pdm_mock_parent_release(struct device *dev)
{
	struct pdm_mock_device_entry *entry;

	entry = container_of(dev, struct pdm_mock_device_entry, parent);
	entry->parent_registered = false;
}

static void pdm_mock_unregister_one(struct pdm_mock_device_entry *entry)
{
	if (entry->pdm_dev) {
		pdm_device_unregister(entry->pdm_dev);
		entry->pdm_dev = NULL;
	}

	if (entry->parent_registered) {
		device_unregister(&entry->parent);
		entry->parent_registered = false;
	}
}

static int pdm_mock_register_parent(struct pdm_mock_device_entry *entry)
{
	int ret;

	device_initialize(&entry->parent);
	entry->parent.release = pdm_mock_parent_release;

	ret = dev_set_name(&entry->parent, "pdm-mock-%s%d",
			   entry->desc->name, entry->desc->id);
	if (ret) {
		put_device(&entry->parent);
		return ret;
	}

	ret = device_add(&entry->parent);
	if (ret) {
		put_device(&entry->parent);
		return ret;
	}

	entry->parent_registered = true;
	return 0;
}

static int pdm_mock_register_one(struct pdm_mock_device_entry *entry,
				 const struct pdm_mock_device_desc *desc)
{
	struct pdm_device *pdm_dev;
	char name[64];
	int ret;

	entry->desc = desc;

	ret = pdm_mock_register_parent(entry);
	if (ret) {
		return ret;
	}

	pdm_dev = pdm_device_alloc(0);
	if (!pdm_dev) {
		ret = -ENOMEM;
		goto err_unregister_parent;
	}

	pdm_dev->dev.parent = &entry->parent;
	pdm_dev->compatible = desc->compatible;
	pdm_device_set_requested_id(pdm_dev, desc->id);

	snprintf(name, sizeof(name), "%s.%s.%d", dev_name(&entry->parent),
		 desc->name, desc->id);

	ret = pdm_device_register(pdm_dev, name);
	if (ret) {
		goto err_unregister_parent;
	}

	entry->pdm_dev = pdm_dev;
	LOG_INFO("Created mock device: %s (%s)", name,
		 desc->compatible);
	return 0;

err_unregister_parent:
	device_unregister(&entry->parent);
	entry->parent_registered = false;
	return ret;
}

int pdm_mock_devices_init(void)
{
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(pdm_mock_device_descs); i++) {
		ret = pdm_mock_register_one(&pdm_mock_devices[i],
					    &pdm_mock_device_descs[i]);
		if (ret) {
			goto err_unregister;
		}
	}

	LOG_INFO("PDM mock devices initialized");
	return 0;

err_unregister:
	while (--i >= 0) {
		pdm_mock_unregister_one(&pdm_mock_devices[i]);
	}
	return ret;
}

void pdm_mock_devices_exit(void)
{
	int i;

	for (i = ARRAY_SIZE(pdm_mock_devices) - 1; i >= 0; i--) {
		pdm_mock_unregister_one(&pdm_mock_devices[i]);
	}

	LOG_INFO("PDM mock devices exited");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM synthetic bus devices for smoke tests");
