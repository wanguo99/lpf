// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_cdev.c
 * @brief PDM per-instance character device helper
 */

#include <linux/err.h>
#include <linux/idr.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>

#include "pdm/core/chardev/pdm_cdev.h"
#include "osal.h"

static dev_t pdm_cdev_devt;
static DEFINE_IDA(pdm_cdev_ida);

static umode_t pdm_cdev_devnode_mode(void)
{
	unsigned int mode = 0660;

#ifdef CONFIG_PDM_INSTANCE_DEVNODE_MODE
	if (kstrtouint(CONFIG_PDM_INSTANCE_DEVNODE_MODE, 8, &mode) != 0) {
		mode = 0660;
	}
#endif

	return (umode_t)(mode & 0777U);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
static char *pdm_cdev_devnode(struct device *dev, umode_t *mode)
#else
static char *pdm_cdev_devnode(const struct device *dev, umode_t *mode)
#endif
{
	if (mode) {
		*mode = pdm_cdev_devnode_mode();
	}

	return kasprintf(GFP_KERNEL, "pdm/%s", dev_name(dev));
}

static struct class pdm_cdev_class = {
	.name = "pdm",
	.devnode = pdm_cdev_devnode,
};

static void pdm_cdev_device_release(struct device *dev)
{
	struct pdm_cdev *client = container_of(dev, struct pdm_cdev, dev);
	void (*release)(struct pdm_cdev *client) = client->release;

	if (client->pdm_dev) {
		pdm_device_put(client->pdm_dev);
		client->pdm_dev = NULL;
	}

	if (client->minor >= 0) {
		ida_free(&pdm_cdev_ida, client->minor);
		client->minor = -1;
	}

	if (release) {
		release(client);
	}
}

int pdm_cdev_default_open(struct inode *inode, struct file *filp)
{
	struct pdm_cdev *client;

	if (!inode || !filp) {
		return -EINVAL;
	}

	client = container_of(inode->i_cdev, struct pdm_cdev, cdev);
	if (!get_device(&client->dev)) {
		return -ENODEV;
	}

	atomic_inc(&client->open_count);
	filp->private_data = client;
	return 0;
}
EXPORT_SYMBOL_GPL(pdm_cdev_default_open);

int pdm_cdev_default_release(struct inode *inode, struct file *filp)
{
	struct pdm_cdev *client = pdm_cdev_from_file(filp);

	(void)inode;
	if (client) {
		atomic_dec_if_positive(&client->open_count);
		filp->private_data = NULL;
		put_device(&client->dev);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(pdm_cdev_default_release);

int pdm_cdev_register(struct pdm_cdev *client, struct pdm_device *pdm_dev,
			const char *name, const char *nodename,
			const struct file_operations *fops,
			void (*release)(struct pdm_cdev *client))
{
	int minor;
	int ret;

	if (!client || !pdm_dev || !name || !nodename || !fops) {
		return -EINVAL;
	}
	if (strchr(nodename, '/')) {
		return -EINVAL;
	}

	minor = ida_alloc_max(&pdm_cdev_ida, PDM_CDEV_MINORS - 1,
				    GFP_KERNEL);
	if (minor < 0) {
		return minor;
	}

	memset(client, 0, sizeof(*client));
	client->minor = -1;
	strscpy(client->name, name, sizeof(client->name));
	strscpy(client->nodename, nodename, sizeof(client->nodename));
	atomic_set(&client->open_count, 0);
	client->minor = minor;
	client->fops = fops;
	client->dev.class = &pdm_cdev_class;
	client->dev.parent = &pdm_dev->dev;
	client->dev.release = pdm_cdev_device_release;
	client->dev.devt = MKDEV(MAJOR(pdm_cdev_devt), minor);
	device_initialize(&client->dev);

	client->pdm_dev = pdm_device_get(pdm_dev);
	if (!client->pdm_dev) {
		ret = -ENODEV;
		goto err_put_device;
	}

	ret = dev_set_name(&client->dev, "%s", client->nodename);
	if (ret) {
		goto err_put_device;
	}

	cdev_init(&client->cdev, client->fops);
	client->cdev.owner = client->fops->owner;

	ret = cdev_device_add(&client->cdev, &client->dev);
	if (ret) {
		LOG_ERROR("Failed to register /dev/pdm/%s: %d",
			  client->nodename, ret);
		goto err_put_device;
	}

	client->release = release;
	client->registered = true;
	LOG_INFO("/dev/pdm/%s registered", client->nodename);
	return 0;

err_put_device:
	put_device(&client->dev);
	return ret;
}
EXPORT_SYMBOL_GPL(pdm_cdev_register);

void pdm_cdev_unregister(struct pdm_cdev *client)
{
	if (!client || !client->registered) {
		return;
	}

	client->registered = false;
	cdev_device_del(&client->cdev, &client->dev);
	LOG_INFO("/dev/pdm/%s unregistered", client->nodename);
	put_device(&client->dev);
}
EXPORT_SYMBOL_GPL(pdm_cdev_unregister);

u32 pdm_cdev_open_count(const struct pdm_cdev *client)
{
	return client ? (u32)atomic_read(&client->open_count) : 0;
}
EXPORT_SYMBOL_GPL(pdm_cdev_open_count);

struct pdm_cdev *pdm_cdev_from_file(struct file *filp)
{
	return filp ? filp->private_data : NULL;
}
EXPORT_SYMBOL_GPL(pdm_cdev_from_file);

int pdm_cdev_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&pdm_cdev_devt, 0, PDM_CDEV_MINORS,
				  "pdm_cdev");
	if (ret) {
		LOG_ERROR("Failed to allocate device region: %d",
			  ret);
		return ret;
	}

	ret = class_register(&pdm_cdev_class);
	if (ret) {
		LOG_ERROR("Failed to register client class: %d",
			  ret);
		unregister_chrdev_region(pdm_cdev_devt, PDM_CDEV_MINORS);
		return ret;
	}

	LOG_INFO("PDM client class initialized");
	return 0;
}

void pdm_cdev_exit(void)
{
	class_unregister(&pdm_cdev_class);
	unregister_chrdev_region(pdm_cdev_devt, PDM_CDEV_MINORS);
	ida_destroy(&pdm_cdev_ida);
	LOG_INFO("PDM client class exited");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM client character device helper");
