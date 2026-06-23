// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_ctl.c
 * @brief PDM control/discovery misc device backed by the PDM bus
 */

#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "pdm_ctl.h"

#include "pdm/core/pdm_bus.h"
#include "pdm/core/pdm_device.h"
#include "pdm/pdm_ctl.h"
#include "pdm/pdm_errno.h"
#include "osal.h"

struct pdm_ctl_match_ctx {
	struct pdm_ctl_device_info *info;
	const char *name;
	u64 required_capabilities;
	u32 match_index;
	u32 seen;
	bool found;
};

static atomic_t pdm_ctl_open_count = ATOMIC_INIT(0);


static bool pdm_ctl_device_visible(const struct pdm_device *pdm_dev)
{
	if (!pdm_dev || !pdm_dev->id_allocated ||
	    pdm_dev->type == PDM_CTL_DEVICE_TYPE_INVALID)
		return false;

	return pdm_dev->state == PDM_CTL_DEVICE_STATE_BOUND ||
	       pdm_dev->state == PDM_CTL_DEVICE_STATE_ERROR;
}

static void pdm_ctl_fill_device_info(struct device *dev,
				     struct pdm_ctl_device_info *info)
{
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	memset(info, 0, sizeof(*info));
	info->type = pdm_dev->type;
	info->index = pdm_dev->id < 0 ? 0 : (u32)pdm_dev->id;
	info->state = pdm_dev->state;
	info->last_error = pdm_dev->last_error;
	info->error_count = pdm_dev->error_count;
	info->capabilities = pdm_dev->capabilities;
	strscpy(info->name, dev_name(dev), sizeof(info->name));
	if (dev->driver)
		strscpy(info->driver_name, dev->driver->name,
			sizeof(info->driver_name));
}

static int pdm_ctl_count_cb(struct device *dev, void *data)
{
	u32 *count = data;
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	if (!pdm_ctl_device_visible(pdm_dev))
		return 0;

	(*count)++;
	return 0;
}

static int pdm_ctl_match_index_cb(struct device *dev, void *data)
{
	struct pdm_ctl_match_ctx *ctx = data;
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	if (!pdm_ctl_device_visible(pdm_dev))
		return 0;

	if (ctx->seen++ != ctx->match_index)
		return 0;

	pdm_ctl_fill_device_info(dev, ctx->info);
	ctx->found = true;
	return 1;
}

static int pdm_ctl_match_name_cb(struct device *dev, void *data)
{
	struct pdm_ctl_match_ctx *ctx = data;
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	if (!pdm_ctl_device_visible(pdm_dev))
		return 0;

	if (strncmp(dev_name(dev), ctx->name, PDM_CTL_NAME_LEN) != 0)
		return 0;

	pdm_ctl_fill_device_info(dev, ctx->info);
	ctx->found = true;
	return 1;
}

static int pdm_ctl_match_capability_cb(struct device *dev, void *data)
{
	struct pdm_ctl_match_ctx *ctx = data;
	struct pdm_device *pdm_dev = dev_to_pdm_device(dev);

	if (!pdm_ctl_device_visible(pdm_dev))
		return 0;

	if ((pdm_dev->capabilities & ctx->required_capabilities) !=
	    ctx->required_capabilities)
		return 0;

	if (ctx->seen++ != ctx->match_index)
		return 0;

	pdm_ctl_fill_device_info(dev, ctx->info);
	ctx->found = true;
	return 1;
}

static long pdm_ctl_get_info(unsigned long arg)
{
	struct pdm_ctl_info info;
	u32 device_count = 0;
	int ret;

	memset(&info, 0, sizeof(info));
	info.abi_version = PDM_CTL_ABI_VERSION;
	info.module_version_major = 1;
	info.module_version_minor = 0;
	info.module_version_patch = 0;
	info.open_count = (u32)atomic_read(&pdm_ctl_open_count);

	ret = pdm_bus_for_each_dev(&device_count, pdm_ctl_count_cb);
	if (ret < 0)
		return ret;
	info.device_count = device_count;

	if (copy_to_user((void __user *)arg, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static long pdm_ctl_get_device(unsigned long arg)
{
	struct pdm_ctl_device_query query;
	struct pdm_ctl_match_ctx ctx;
	int ret;

	if (copy_from_user(&query, (void __user *)arg, sizeof(query)))
		return -EFAULT;

	memset(&ctx, 0, sizeof(ctx));
	ctx.info = &query.info;
	ctx.match_index = query.match_index;

	ret = pdm_bus_for_each_dev(&ctx, pdm_ctl_match_index_cb);
	if (ret < 0)
		return ret;
	if (!ctx.found)
		return -ENODEV;

	if (copy_to_user((void __user *)arg, &query, sizeof(query)))
		return -EFAULT;

	return 0;
}

static long pdm_ctl_get_device_by_name(unsigned long arg)
{
	struct pdm_ctl_device_name_query query;
	struct pdm_ctl_match_ctx ctx;
	int ret;

	if (copy_from_user(&query, (void __user *)arg, sizeof(query)))
		return -EFAULT;
	query.name[PDM_CTL_NAME_LEN - 1] = '\0';

	memset(&ctx, 0, sizeof(ctx));
	ctx.info = &query.info;
	ctx.name = query.name;

	ret = pdm_bus_for_each_dev(&ctx, pdm_ctl_match_name_cb);
	if (ret < 0)
		return ret;
	if (!ctx.found)
		return -ENODEV;

	if (copy_to_user((void __user *)arg, &query, sizeof(query)))
		return -EFAULT;

	return 0;
}

static long pdm_ctl_get_device_by_capability(unsigned long arg)
{
	struct pdm_ctl_device_query query;
	struct pdm_ctl_match_ctx ctx;
	int ret;

	if (copy_from_user(&query, (void __user *)arg, sizeof(query)))
		return -EFAULT;
	if (!query.required_capabilities)
		return -EINVAL;

	memset(&ctx, 0, sizeof(ctx));
	ctx.info = &query.info;
	ctx.required_capabilities = query.required_capabilities;
	ctx.match_index = query.match_index;

	ret = pdm_bus_for_each_dev(&ctx, pdm_ctl_match_capability_cb);
	if (ret < 0)
		return ret;
	if (!ctx.found)
		return -ENODEV;

	if (copy_to_user((void __user *)arg, &query, sizeof(query)))
		return -EFAULT;

	return 0;
}

static long pdm_ctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	(void)filp;

	switch (cmd) {
	case PDM_CTL_IOC_GET_INFO:
		return pdm_ctl_get_info(arg);
	case PDM_CTL_IOC_GET_DEVICE:
		return pdm_ctl_get_device(arg);
	case PDM_CTL_IOC_GET_DEVICE_BY_NAME:
		return pdm_ctl_get_device_by_name(arg);
	case PDM_CTL_IOC_GET_DEVICE_BY_CAPABILITY:
		return pdm_ctl_get_device_by_capability(arg);
	default:
		return -ENOTTY;
	}
}

static int pdm_ctl_open(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;
	atomic_inc(&pdm_ctl_open_count);
	return 0;
}

static int pdm_ctl_release(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;
	atomic_dec_if_positive(&pdm_ctl_open_count);
	return 0;
}

static const struct file_operations pdm_ctl_fops = {
	.owner = THIS_MODULE,
	.open = pdm_ctl_open,
	.release = pdm_ctl_release,
	.unlocked_ioctl = pdm_ctl_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pdm_ctl_ioctl,
#endif
};

static struct miscdevice pdm_ctl_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = PDM_CTL_DEVICE_NAME,
	.fops = &pdm_ctl_fops,
	.mode = 0666,
};

int pdm_ctl_init(void)
{
	int ret;

	ret = misc_register(&pdm_ctl_miscdev);
	if (ret) {
		LOG_ERROR("PDM-CTL", "Failed to register /dev/%s: %d",
			  PDM_CTL_DEVICE_NAME, ret);
		return ret;
	}

	LOG_INFO("PDM-CTL", "/dev/%s registered", PDM_CTL_DEVICE_NAME);
	return 0;
}

void pdm_ctl_exit(void)
{
	misc_deregister(&pdm_ctl_miscdev);
	LOG_INFO("PDM-CTL", "/dev/%s unregistered", PDM_CTL_DEVICE_NAME);
}
