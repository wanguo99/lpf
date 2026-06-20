// SPDX-License-Identifier: GPL-2.0

#include "pdm_chrdev.h"

#include <linux/container_of.h>
#include <linux/device.h>
#include <linux/errno.h>

#include "lpf/lpf_core.h"
#include "lpf/lpf_driver.h"
#include "lpf/lpf_soc_adapter.h"

static ssize_t name_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "\n");

	return sysfs_emit(buf, "%s\n", chrdev->info.name);
}

static ssize_t type_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "0\n");

	return sysfs_emit(buf, "%u\n", chrdev->info.type);
}

static ssize_t index_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "0\n");

	return sysfs_emit(buf, "%u\n", chrdev->index);
}

static ssize_t state_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "0\n");

	return sysfs_emit(buf, "%u\n", chrdev->info.state);
}

static ssize_t capabilities_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "0x0\n");

	return sysfs_emit(buf, "0x%llx\n",
			  (unsigned long long)chrdev->info.capabilities);
}

static ssize_t driver_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "\n");

	return sysfs_emit(buf, "%s\n", chrdev->info.driver_name);
}

static ssize_t soc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "\n");

	return sysfs_emit(buf, "%s\n", chrdev->soc_name);
}

static ssize_t last_error_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "0\n");

	return sysfs_emit(buf, "%d\n", chrdev->info.last_error);
}

static ssize_t error_count_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "0\n");

	return sysfs_emit(buf, "%u\n", pdm_chrdev_error_count(chrdev));
}

static ssize_t open_count_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	pdm_chrdev_t *chrdev = dev_get_drvdata(dev);

	(void)attr;
	if (!chrdev)
		return sysfs_emit(buf, "0\n");

	return sysfs_emit(buf, "%u\n", pdm_chrdev_open_count(chrdev));
}

static DEVICE_ATTR_RO(name);
static DEVICE_ATTR_RO(type);
static DEVICE_ATTR_RO(index);
static DEVICE_ATTR_RO(state);
static DEVICE_ATTR_RO(capabilities);
static DEVICE_ATTR_RO(driver);
static DEVICE_ATTR_RO(soc);
static DEVICE_ATTR_RO(last_error);
static DEVICE_ATTR_RO(error_count);
static DEVICE_ATTR_RO(open_count);

static struct attribute *pdm_chrdev_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_type.attr,
	&dev_attr_index.attr,
	&dev_attr_state.attr,
	&dev_attr_capabilities.attr,
	&dev_attr_driver.attr,
	&dev_attr_soc.attr,
	&dev_attr_last_error.attr,
	&dev_attr_error_count.attr,
	&dev_attr_open_count.attr,
	NULL,
};

static const struct attribute_group pdm_chrdev_attr_group = {
	.attrs = pdm_chrdev_attrs,
};

static const struct attribute_group *pdm_chrdev_attr_groups[] = {
	&pdm_chrdev_attr_group,
	NULL,
};

typedef struct {
	pdm_chrdev_t *chrdev;
	lpf_device_handle_t *device_handle;
} pdm_chrdev_file_ctx_t;

static pdm_chrdev_t *pdm_chrdev_from_private_data(void *private_data)
{
	struct miscdevice *miscdev;

	if (!private_data)
		return NULL;

	miscdev = private_data;
	return container_of(miscdev, pdm_chrdev_t, miscdev);
}

static void pdm_chrdev_fill_info(pdm_chrdev_t *chrdev,
				 const lpf_device_t *device, uint32_t index)
{
	const char *soc_name;

	osal_memset(&chrdev->info, 0, sizeof(chrdev->info));
	chrdev->info.index = index;

	if (device) {
		chrdev->info.type = device->config.type;
		chrdev->info.index = device->config.index;
		chrdev->info.state = device->state;
		chrdev->info.last_error = device->last_error;
		chrdev->info.error_count = device->error_count;
		chrdev->info.capabilities = device->capabilities;
		osal_strncpy(chrdev->info.name, device->name,
			     sizeof(chrdev->info.name) - 1U);
		chrdev->info.name[sizeof(chrdev->info.name) - 1U] = '\0';
		if (device->driver && device->driver->name) {
			osal_strncpy(chrdev->info.driver_name,
				     device->driver->name,
				     sizeof(chrdev->info.driver_name) - 1U);
			chrdev->info.driver_name[
				sizeof(chrdev->info.driver_name) - 1U] = '\0';
		}
	}

	soc_name = lpf_soc_adapter_name();
	osal_strncpy(chrdev->soc_name, soc_name ? soc_name : "unknown",
		     sizeof(chrdev->soc_name) - 1U);
	chrdev->soc_name[sizeof(chrdev->soc_name) - 1U] = '\0';
}

int pdm_chrdev_open(struct file *file)
{
	pdm_chrdev_file_ctx_t *ctx;
	lpf_device_handle_t *handle = NULL;
	pdm_chrdev_t *chrdev;
	int open_count;

	if (!file)
		return -EINVAL;

	chrdev = pdm_chrdev_from_private_data(file->private_data);
	if (!chrdev)
		return -EINVAL;

	ctx = osal_zalloc(sizeof(*ctx));
	if (!ctx)
		return -ENOMEM;

	osal_mutex_lock(&chrdev->lock);
	if (chrdev->info.type != LPF_DEVICE_TYPE_INVALID) {
		handle = lpf_device_get(chrdev->info.type, chrdev->info.index);
		if (!handle) {
			osal_mutex_unlock(&chrdev->lock);
			osal_free(ctx);
			return -ENODEV;
		}
	}
	ctx->chrdev = chrdev;
	ctx->device_handle = handle;
	file->private_data = ctx;
	open_count = (int)osal_atomic_inc(&chrdev->open_count);
	osal_mutex_unlock(&chrdev->lock);

	LOG_INFO(chrdev->name, "open count=%d", open_count);
	return 0;
}

int pdm_chrdev_release(struct file *file)
{
	pdm_chrdev_file_ctx_t *ctx;
	lpf_device_handle_t *handle = NULL;
	pdm_chrdev_t *chrdev;
	int open_count;

	if (!file || !file->private_data)
		return -EINVAL;

	ctx = file->private_data;
	chrdev = ctx->chrdev;
	handle = ctx->device_handle;
	file->private_data = NULL;

	if (!chrdev) {
		lpf_device_put(handle);
		osal_free(ctx);
		return -EINVAL;
	}

	osal_mutex_lock(&chrdev->lock);
	if (osal_atomic_load(&chrdev->open_count) > 0)
		osal_atomic_dec(&chrdev->open_count);
	open_count = (int)osal_atomic_load(&chrdev->open_count);
	osal_mutex_unlock(&chrdev->lock);

	lpf_device_put(handle);
	osal_free(ctx);
	LOG_INFO(chrdev->name, "release count=%d", open_count);
	return 0;
}

int pdm_chrdev_register(pdm_chrdev_t *chrdev, const char *name,
			const struct file_operations *fops)
{
	return pdm_chrdev_register_instance(chrdev, name, NULL, 0, fops);
}

int pdm_chrdev_register_instance(pdm_chrdev_t *chrdev, const char *name,
				 const char *nodename, uint32_t index,
				 const struct file_operations *fops)
{
	lpf_device_t device;

	osal_memset(&device, 0, sizeof(device));
	device.config.index = index;
	device.config.name = name;
	osal_strncpy(device.name, name, sizeof(device.name) - 1U);
	device.name[sizeof(device.name) - 1U] = '\0';

	return pdm_chrdev_register_lpf_device(chrdev, name, nodename, &device,
					      fops);
}

int pdm_chrdev_register_lpf_device(pdm_chrdev_t *chrdev, const char *name,
				   const char *nodename,
				   const lpf_device_t *device,
				   const struct file_operations *fops)
{
	int ret;
	uint32_t index;

	if (!chrdev || !name || !fops)
		return -EINVAL;
	if (chrdev->registered)
		return -EBUSY;

	index = device ? device->config.index : 0;

	ret = osal_mutex_init(&chrdev->lock, NULL);
	if (ret != OSAL_SUCCESS)
		return -ret;

	osal_strncpy(chrdev->name, name, sizeof(chrdev->name));
	chrdev->name[sizeof(chrdev->name) - 1U] = '\0';
	if (nodename) {
		osal_strncpy(chrdev->nodename, nodename,
			     sizeof(chrdev->nodename));
		chrdev->nodename[sizeof(chrdev->nodename) - 1U] = '\0';
	}
	chrdev->fops = fops;
	chrdev->index = index;
	pdm_chrdev_fill_info(chrdev, device, index);
	osal_atomic_init(&chrdev->open_count, 0);
	osal_atomic_init(&chrdev->error_count, 0);

	chrdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	chrdev->miscdev.name = chrdev->name;
	chrdev->miscdev.fops = fops;
	chrdev->miscdev.nodename = nodename ? chrdev->nodename : NULL;
	chrdev->miscdev.groups = pdm_chrdev_attr_groups;
	chrdev->miscdev.mode = 0666;

	ret = misc_register(&chrdev->miscdev);
	if (ret) {
		osal_mutex_destroy(&chrdev->lock);
		return ret;
	}

	if (chrdev->miscdev.this_device)
		dev_set_drvdata(chrdev->miscdev.this_device, chrdev);

	chrdev->registered = true;
	LOG_INFO(chrdev->name, "/dev/%s ready",
		 chrdev->miscdev.nodename ? chrdev->miscdev.nodename :
					    chrdev->name);
	return 0;
}

void pdm_chrdev_unregister(pdm_chrdev_t *chrdev)
{
	if (!chrdev)
		return;
	if (!chrdev->registered)
		return;

	misc_deregister(&chrdev->miscdev);
	osal_mutex_destroy(&chrdev->lock);
	osal_memset(chrdev, 0, sizeof(*chrdev));
}

pdm_chrdev_t *pdm_chrdev_from_file(struct file *file)
{
	pdm_chrdev_file_ctx_t *ctx;
	pdm_chrdev_t *chrdev;

	if (!file || !file->private_data)
		return NULL;

	ctx = file->private_data;
	chrdev = ctx->chrdev;
	return chrdev && chrdev->registered ? chrdev : NULL;
}

uint32_t pdm_chrdev_open_count(const pdm_chrdev_t *chrdev)
{
	if (!chrdev)
		return 0;

	return osal_atomic_load(&chrdev->open_count);
}

uint32_t pdm_chrdev_error_count(const pdm_chrdev_t *chrdev)
{
	if (!chrdev)
		return 0;

	return osal_atomic_load(&chrdev->error_count);
}

void pdm_chrdev_record_error(pdm_chrdev_t *chrdev, int error)
{
	if (!chrdev || error == 0)
		return;
	if (!chrdev->registered)
		return;

	osal_mutex_lock(&chrdev->lock);
	chrdev->info.last_error = error;
	chrdev->info.error_count = osal_atomic_inc(&chrdev->error_count);
	osal_mutex_unlock(&chrdev->lock);

	lpf_device_record_error(chrdev->info.type, chrdev->info.index, error);
}

uint32_t pdm_chrdev_index(const pdm_chrdev_t *chrdev)
{
	if (!chrdev)
		return 0;

	return chrdev->index;
}
