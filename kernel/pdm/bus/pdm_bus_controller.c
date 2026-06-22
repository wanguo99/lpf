// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_bus_controller.c
 * @brief PDM Bus Controller - Creates PDM devices from Device Tree
 *
 * This platform driver reads Device Tree nodes under "vendor,pdm-bus"
 * and creates PDM devices on the PDM bus for each child node.
 *
 * Similar to platform_drv_probe_with_id() for platform devices.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/slab.h>

#include "pdm/core/pdm_bus.h"
#include "pdm/core/pdm_device_new.h"
#include "osal.h"

/**
 * @brief PDM bus controller data
 *
 * Tracks all PDM devices created by this controller
 */
struct pdm_bus_controller {
	struct platform_device *pdev;
	struct list_head devices;	/* List of created pdm_device pointers */
	int device_count;
};

struct pdm_device_list_entry {
	struct list_head node;
	struct pdm_device *pdm_dev;
};

/**
 * @brief Probe function - called when DT node matches
 *
 * This function is called when a Device Tree node with compatible
 * "vendor,pdm-bus" is found. It iterates through child nodes and
 * creates a PDM device for each one.
 */
static int pdm_bus_controller_probe(struct platform_device *pdev)
{
	struct pdm_bus_controller *ctrl;
	struct device_node *child;
	int ret = 0;
	int id = 0;

	LOG_INFO("PDM-BUS-CTRL", "Probing PDM bus controller");

	ctrl = devm_kzalloc(&pdev->dev, sizeof(*ctrl), GFP_KERNEL);
	if (!ctrl)
		return -ENOMEM;

	ctrl->pdev = pdev;
	INIT_LIST_HEAD(&ctrl->devices);
	platform_set_drvdata(pdev, ctrl);

	/* Iterate through all child nodes in Device Tree */
	for_each_available_child_of_node(pdev->dev.of_node, child) {
		struct pdm_device *pdm_dev;
		struct pdm_device_list_entry *entry;
		const char *compatible;
		char name[32];

		/* Get compatible string */
		ret = of_property_read_string(child, "compatible", &compatible);
		if (ret) {
			LOG_WARN("PDM-BUS-CTRL",
				 "Child node %s has no compatible string, skipping",
				 child->name);
			continue;
		}

		/* Allocate PDM device */
		pdm_dev = pdm_device_alloc(0);
		if (!pdm_dev) {
			LOG_ERROR("PDM-BUS-CTRL",
				  "Failed to allocate PDM device for %s",
				  child->name);
			ret = -ENOMEM;
			goto err_cleanup;
		}

		/* Set device properties */
		pdm_dev->dev.parent = &pdev->dev;
		pdm_dev->dev.of_node = child;
		pdm_dev->compatible = compatible;
		pdm_dev->id = id++;

		/* Generate device name */
		snprintf(name, sizeof(name), "%s", child->name);

		/* Register the device on PDM bus */
		ret = pdm_device_register(pdm_dev, name);
		if (ret) {
			LOG_ERROR("PDM-BUS-CTRL",
				  "Failed to register device %s, error %d",
				  name, ret);
			put_device(&pdm_dev->dev);
			goto err_cleanup;
		}

		/* Track this device for cleanup */
		entry = devm_kzalloc(&pdev->dev, sizeof(*entry), GFP_KERNEL);
		if (entry) {
			entry->pdm_dev = pdm_dev;
			list_add_tail(&entry->node, &ctrl->devices);
			ctrl->device_count++;
		}

		LOG_INFO("PDM-BUS-CTRL",
			 "Created device: %s (compatible: %s)",
			 name, compatible);
	}

	LOG_INFO("PDM-BUS-CTRL",
		 "PDM bus controller probe complete, %d devices created",
		 ctrl->device_count);

	return 0;

err_cleanup:
	/* Cleanup any devices we created before the error */
	{
		struct pdm_device_list_entry *entry, *tmp;
		list_for_each_entry_safe(entry, tmp, &ctrl->devices, node) {
			pdm_device_unregister(entry->pdm_dev);
			list_del(&entry->node);
		}
	}
	of_node_put(child);
	return ret;
}

/**
 * @brief Remove function - cleanup when module unloads
 */
static void pdm_bus_controller_remove(struct platform_device *pdev)
{
	struct pdm_bus_controller *ctrl = platform_get_drvdata(pdev);
	struct pdm_device_list_entry *entry, *tmp;

	LOG_INFO("PDM-BUS-CTRL", "Removing PDM bus controller");

	/* Unregister all PDM devices */
	list_for_each_entry_safe(entry, tmp, &ctrl->devices, node) {
		LOG_DEBUG("PDM-BUS-CTRL", "Unregistering device: %s",
			  dev_name(&entry->pdm_dev->dev));
		pdm_device_unregister(entry->pdm_dev);
		list_del(&entry->node);
	}

	LOG_INFO("PDM-BUS-CTRL", "PDM bus controller removed");
}

/**
 * @brief Device Tree match table
 *
 * This driver matches Device Tree nodes with compatible = "vendor,pdm-bus"
 */
static const struct of_device_id pdm_bus_controller_of_match[] = {
	{ .compatible = "vendor,pdm-bus" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_bus_controller_of_match);

/**
 * @brief Platform driver structure
 */
static struct platform_driver pdm_bus_controller_driver = {
	.probe = pdm_bus_controller_probe,
	.remove = pdm_bus_controller_remove,
	.driver = {
		.name = "pdm-bus-controller",
		.of_match_table = pdm_bus_controller_of_match,
	},
};

/**
 * @brief Module init - register platform driver
 */
int __init pdm_bus_controller_init(void)
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

/**
 * @brief Module exit - unregister platform driver
 */
void __exit pdm_bus_controller_exit(void)
{
	platform_driver_unregister(&pdm_bus_controller_driver);
	LOG_INFO("PDM-BUS-CTRL", "PDM bus controller driver unregistered");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("PDM Team");
MODULE_DESCRIPTION("PDM Bus Controller - Creates devices from Device Tree");
MODULE_SOFTDEP("pre: pdm_core");
