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
#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/uaccess.h>

#include "pdm/bus/pdm_cdev.h"
#include "pdm/bus/pdm_bus.h"
#include "pdm/bus/pdm_device.h"
#include "pdm/pdm_manager.h"
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

/* ========================================================================
 * PDM Manager - Device Discovery and Enumeration
 * ======================================================================== */

struct pdm_manager_match_ctx {
	struct pdm_manager_device_info *info;
	const char *name;
	u64 required_capabilities;
	u32 match_index;
	u32 seen;
	bool found;
};

static atomic_t pdm_manager_open_count = ATOMIC_INIT(0);

static bool pdm_manager_device_visible(const struct pdm_device *pdm_dev)
{
	if (!pdm_dev || !pdm_dev->id_allocated ||
	    pdm_dev->type == PDM_MANAGER_DEVICE_TYPE_INVALID) {
		return false;
	}

	if (pdm_dev->owner == PDM_MANAGER_DEVICE_OWNER_USER &&
	    pdm_dev->state == PDM_MANAGER_DEVICE_STATE_REGISTERED) {
		return true;
	}

	return pdm_dev->state == PDM_MANAGER_DEVICE_STATE_BOUND ||
	       pdm_dev->state == PDM_MANAGER_DEVICE_STATE_ERROR;
}

static void pdm_manager_copy_of_path(struct device_node *np, char *buf,
				     size_t len)
{
	if (!buf || !len) {
		return;
	}

	buf[0] = '\0';
	if (np) {
		snprintf(buf, len, "%pOF", np);
	}
}

static void pdm_manager_fill_device_info(struct device *dev,
					 struct pdm_manager_device_info *info)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	memset(info, 0, sizeof(*info));
	info->type = pdm_dev->type;
	info->index = pdm_dev->id;
	info->state = pdm_dev->state;
	info->last_error = pdm_dev->last_error;
	info->error_count = pdm_dev->error_count;
	info->owner = pdm_dev->owner;
	info->transport = pdm_dev->transport;
	info->capabilities = pdm_dev->capabilities;
	pdm_manager_copy_of_path(dev->of_node, info->of_node_path,
				 PDM_MANAGER_PATH_LEN);
	pdm_manager_copy_of_path(pdm_dev->controller_node,
				 info->controller_path, PDM_MANAGER_PATH_LEN);

	strscpy(info->name, dev_name(dev), PDM_MANAGER_NAME_LEN);
	if (dev->driver && dev->driver->name) {
		strscpy(info->driver_name, dev->driver->name,
			PDM_MANAGER_NAME_LEN);
	}
}

static int pdm_manager_match_device(struct device *dev, void *data)
{
	struct pdm_manager_match_ctx *ctx = data;
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	if (!pdm_manager_device_visible(pdm_dev)) {
		return 0;
	}

	if (ctx->seen++ == ctx->match_index) {
		pdm_manager_fill_device_info(dev, ctx->info);
		ctx->found = true;
		return 1;
	}

	return 0;
}

static int pdm_manager_match_by_name(struct device *dev, void *data)
{
	struct pdm_manager_match_ctx *ctx = data;
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	if (!pdm_manager_device_visible(pdm_dev)) {
		return 0;
	}

	if (strcmp(dev_name(dev), ctx->name) == 0) {
		pdm_manager_fill_device_info(dev, ctx->info);
		ctx->found = true;
		return 1;
	}

	return 0;
}

static int pdm_manager_match_by_capability(struct device *dev, void *data)
{
	struct pdm_manager_match_ctx *ctx = data;
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	if (!pdm_manager_device_visible(pdm_dev)) {
		return 0;
	}

	if ((pdm_dev->capabilities & ctx->required_capabilities) ==
	    ctx->required_capabilities) {
		if (ctx->seen++ == ctx->match_index) {
			pdm_manager_fill_device_info(dev, ctx->info);
			ctx->found = true;
			return 1;
		}
	}

	return 0;
}

static int pdm_manager_count_visible(struct device *dev, void *data)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);
	u32 *count = data;

	if (pdm_manager_device_visible(pdm_dev)) {
		(*count)++;
	}

	return 0;
}

static int pdm_manager_do_get_info(struct pdm_manager_info __user *uinfo)
{
	struct pdm_manager_info info;
	u32 device_count = 0;

	memset(&info, 0, sizeof(info));
	info.abi_version = PDM_MANAGER_ABI_VERSION;
	info.module_version_major = 1;
	info.module_version_minor = 0;
	info.module_version_patch = 0;
	info.open_count = atomic_read(&pdm_manager_open_count);

	bus_for_each_dev(&pdm_bus_type, NULL, &device_count,
			 pdm_manager_count_visible);
	info.device_count = device_count;

	if (copy_to_user(uinfo, &info, sizeof(info))) {
		return -EFAULT;
	}

	return 0;
}

static int pdm_manager_find_device_by_index(u32 match_index,
						 struct pdm_manager_device_info *info)
{
	struct pdm_manager_match_ctx ctx;

	memset(&ctx, 0, sizeof(ctx));
	ctx.info = info;
	ctx.match_index = match_index;

	bus_for_each_dev(&pdm_bus_type, NULL, &ctx, pdm_manager_match_device);
	return ctx.found ? 0 : -ENODEV;
}

static int pdm_manager_do_get_device(struct pdm_manager_device_query __user *uquery)
{
	struct pdm_manager_device_query query;
	int ret;

	if (copy_from_user(&query, uquery, sizeof(query))) {
		return -EFAULT;
	}

	ret = pdm_manager_find_device_by_index(query.match_index, &query.info);
	if (ret) {
		return ret;
	}

	if (copy_to_user(uquery, &query, sizeof(query))) {
		return -EFAULT;
	}

	return 0;
}

static int pdm_manager_find_device_by_name(const char *name,
					      struct pdm_manager_device_info *info)
{
	struct pdm_manager_match_ctx ctx;

	memset(&ctx, 0, sizeof(ctx));
	ctx.info = info;
	ctx.name = name;

	bus_for_each_dev(&pdm_bus_type, NULL, &ctx,
			 pdm_manager_match_by_name);
	return ctx.found ? 0 : -ENODEV;
}

static int pdm_manager_do_get_device_by_name(
	struct pdm_manager_device_name_query __user *uquery)
{
	struct pdm_manager_device_name_query query;
	int ret;

	if (copy_from_user(&query, uquery, sizeof(query))) {
		return -EFAULT;
	}

	query.name[PDM_MANAGER_NAME_LEN - 1] = '\0';

	ret = pdm_manager_find_device_by_name(query.name, &query.info);
	if (ret) {
		return ret;
	}

	if (copy_to_user(uquery, &query, sizeof(query))) {
		return -EFAULT;
	}

	return 0;
}

static int pdm_manager_find_device_by_capability(u64 required_capabilities,
						      u32 match_index,
						      struct pdm_manager_device_info *info)
{
	struct pdm_manager_match_ctx ctx;

	memset(&ctx, 0, sizeof(ctx));
	ctx.info = info;
	ctx.match_index = match_index;
	ctx.required_capabilities = required_capabilities;

	bus_for_each_dev(&pdm_bus_type, NULL, &ctx,
			 pdm_manager_match_by_capability);
	return ctx.found ? 0 : -ENODEV;
}

static int pdm_manager_do_get_device_by_capability(
	struct pdm_manager_device_query __user *uquery)
{
	struct pdm_manager_device_query query;
	int ret;

	if (copy_from_user(&query, uquery, sizeof(query))) {
		return -EFAULT;
	}

	ret = pdm_manager_find_device_by_capability(query.required_capabilities,
						   query.match_index, &query.info);
	if (ret) {
		return ret;
	}

	if (copy_to_user(uquery, &query, sizeof(query))) {
		return -EFAULT;
	}

	return 0;
}

static long pdm_manager_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;

	switch (cmd) {
	case PDM_MANAGER_IOC_GET_INFO:
		return pdm_manager_do_get_info(uarg);
	case PDM_MANAGER_IOC_GET_DEVICE:
		return pdm_manager_do_get_device(uarg);
	case PDM_MANAGER_IOC_GET_DEVICE_BY_NAME:
		return pdm_manager_do_get_device_by_name(uarg);
	case PDM_MANAGER_IOC_GET_DEVICE_BY_CAPABILITY:
		return pdm_manager_do_get_device_by_capability(uarg);
	default:
		return -ENOTTY;
	}
}

static int pdm_manager_open(struct inode *inode, struct file *filp)
{
	atomic_inc(&pdm_manager_open_count);
	return 0;
}

static int pdm_manager_release(struct inode *inode, struct file *filp)
{
	atomic_dec_if_positive(&pdm_manager_open_count);
	return 0;
}

static const struct file_operations pdm_manager_fops = {
	.owner = THIS_MODULE,
	.open = pdm_manager_open,
	.release = pdm_manager_release,
	.unlocked_ioctl = pdm_manager_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pdm_manager_ioctl,
#endif
};

static struct miscdevice pdm_manager_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = PDM_MANAGER_DEVICE_NAME,
	.fops = &pdm_manager_fops,
	.mode = 0666,
};


int pdm_cdev_init(void)
{
	int ret;

	/* Initialize manager device for discovery/enumeration */
	ret = misc_register(&pdm_manager_miscdev);
	if (ret) {
		LOG_ERROR("Failed to register /dev/%s: %d",
			  PDM_MANAGER_DEVICE_NAME, ret);
		return ret;
	}
	LOG_INFO("/dev/%s registered", PDM_MANAGER_DEVICE_NAME);

	/* Allocate character device region for device instances */
	ret = alloc_chrdev_region(&pdm_cdev_devt, 0, PDM_CDEV_MINORS,
				  "pdm_cdev");
	if (ret) {
		LOG_ERROR("Failed to allocate device region: %d", ret);
		goto err_manager;
	}

	/* Register device class */
	ret = class_register(&pdm_cdev_class);
	if (ret) {
		LOG_ERROR("Failed to register client class: %d", ret);
		goto err_chrdev;
	}

	LOG_INFO("PDM character device subsystem initialized");
	return 0;

err_chrdev:
	unregister_chrdev_region(pdm_cdev_devt, PDM_CDEV_MINORS);
err_manager:
	misc_deregister(&pdm_manager_miscdev);
	return ret;
}

void pdm_cdev_exit(void)
{
	class_unregister(&pdm_cdev_class);
	unregister_chrdev_region(pdm_cdev_devt, PDM_CDEV_MINORS);
	ida_destroy(&pdm_cdev_ida);
	misc_deregister(&pdm_manager_miscdev);
	LOG_INFO("PDM character device subsystem exited");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM character device subsystem");
