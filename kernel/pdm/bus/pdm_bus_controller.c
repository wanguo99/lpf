// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_bus_controller.c
 * @brief PDM Bus Controller - creates PDM devices from Device Tree
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "pdm/core/pdm_bus.h"
#include "pdm/core/pdm_device.h"
#include "pdm_bus_controller.h"
#include "osal.h"

#define PDM_BUS_DEVICE_NAME_LEN 96

struct pdm_bus_controller {
	struct platform_device *pdev;
	struct list_head devices;
	int device_count;
};

struct pdm_device_list_entry {
	struct list_head node;
	struct pdm_device *pdm_dev;
};

static void pdm_bus_controller_unregister_devices(struct pdm_bus_controller *ctrl)
{
	struct pdm_device_list_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, &ctrl->devices, node) {
		LOG_DEBUG("PDM-BUS-CTRL", "Unregistering device: %s",
			  dev_name(&entry->pdm_dev->dev));
		pdm_device_unregister(entry->pdm_dev);
		list_del(&entry->node);
		ctrl->device_count--;
	}
}

static int pdm_bus_controller_child_id(struct device_node *child)
{
	u32 reg;

	if (!of_property_read_u32(child, "pdm,id", &reg))
		return (int)reg;
	if (of_property_read_u32(child, "reg", &reg) == 0)
		return (int)reg;

	return -1;
}

static int pdm_bus_controller_probe(struct platform_device *pdev)
{
	struct pdm_bus_controller *ctrl;
	struct device_node *child;
	int auto_name_id = 0;
	int ret = 0;

	LOG_INFO("PDM-BUS-CTRL", "Probing PDM bus controller");

	ctrl = devm_kzalloc(&pdev->dev, sizeof(*ctrl), GFP_KERNEL);
	if (!ctrl)
		return -ENOMEM;

	ctrl->pdev = pdev;
	INIT_LIST_HEAD(&ctrl->devices);
	platform_set_drvdata(pdev, ctrl);

	for_each_available_child_of_node(pdev->dev.of_node, child) {
		struct pdm_device_list_entry *entry;
		struct pdm_device *pdm_dev;
		const char *compatible;
		char name[PDM_BUS_DEVICE_NAME_LEN];
		int device_id;

		ret = of_property_read_string(child, "compatible", &compatible);
		if (ret) {
			LOG_WARN("PDM-BUS-CTRL",
				 "Child node %s has no compatible string, skipping",
				 child->name);
			continue;
		}

		pdm_dev = pdm_device_alloc(0);
		if (!pdm_dev) {
			ret = -ENOMEM;
			goto err_cleanup;
		}

		entry = devm_kzalloc(&pdev->dev, sizeof(*entry), GFP_KERNEL);
		if (!entry) {
			put_device(&pdm_dev->dev);
			ret = -ENOMEM;
			goto err_cleanup;
		}

		device_id = pdm_bus_controller_child_id(child);
		pdm_dev->dev.parent = &pdev->dev;
		pdm_dev->dev.of_node = of_node_get(child);
		pdm_dev->compatible = compatible;
		pdm_device_set_requested_id(pdm_dev, device_id);

		if (device_id >= 0)
			snprintf(name, sizeof(name), "%s.%s.%d",
				 dev_name(&pdev->dev), child->name, device_id);
		else
			snprintf(name, sizeof(name), "%s.%s.auto%d",
				 dev_name(&pdev->dev), child->name, auto_name_id++);

		ret = pdm_device_register(pdm_dev, name);
		if (ret) {
			LOG_ERROR("PDM-BUS-CTRL",
				  "Failed to register device %s, error %d",
				  name, ret);
			goto err_cleanup;
		}

		entry->pdm_dev = pdm_dev;
		list_add_tail(&entry->node, &ctrl->devices);
		ctrl->device_count++;

		LOG_INFO("PDM-BUS-CTRL", "Created device: %s (compatible: %s)",
			 name, compatible);
	}

	LOG_INFO("PDM-BUS-CTRL",
		 "PDM bus controller probe complete, %d devices created",
		 ctrl->device_count);
	return 0;

err_cleanup:
	pdm_bus_controller_unregister_devices(ctrl);
	of_node_put(child);
	return ret;
}

static void pdm_bus_controller_remove(struct platform_device *pdev)
{
	struct pdm_bus_controller *ctrl = platform_get_drvdata(pdev);

	if (!ctrl)
		return;

	LOG_INFO("PDM-BUS-CTRL", "Removing PDM bus controller");
	pdm_bus_controller_unregister_devices(ctrl);
	LOG_INFO("PDM-BUS-CTRL", "PDM bus controller removed");
}

static const struct of_device_id pdm_bus_controller_of_match[] = {
	{ .compatible = "vendor,pdm-bus" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_bus_controller_of_match);

static struct platform_driver pdm_bus_controller_driver = {
	.probe = pdm_bus_controller_probe,
	.remove = pdm_bus_controller_remove,
	.driver = {
		.name = "pdm-bus-controller",
		.of_match_table = pdm_bus_controller_of_match,
	},
};

int pdm_bus_controller_init(void)
{
	int ret;

	ret = platform_driver_register(&pdm_bus_controller_driver);
	if (ret) {
		LOG_ERROR("PDM-BUS-CTRL",
			  "Failed to register platform driver, error %d", ret);
		return ret;
	}

	LOG_INFO("PDM-BUS-CTRL", "PDM bus controller driver registered");
	return 0;
}

void pdm_bus_controller_exit(void)
{
	platform_driver_unregister(&pdm_bus_controller_driver);
	LOG_INFO("PDM-BUS-CTRL", "PDM bus controller driver unregistered");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PDM Team");
MODULE_DESCRIPTION("PDM Bus Controller - Creates devices from Device Tree");
