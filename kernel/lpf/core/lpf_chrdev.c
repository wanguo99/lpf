// SPDX-License-Identifier: GPL-2.0

#include "lpf/lpf_chrdev.h"

#include <linux/container_of.h>
#include <linux/device.h>
#include <linux/errno.h>

#include "lpf/lpf_soc_adapter.h"
#include "lpf_sysfs.h"

typedef struct {
	lpf_chrdev_t *chrdev;
	lpf_device_handle_t *device_handle;
} lpf_chrdev_file_ctx_t;

static lpf_chrdev_t *lpf_chrdev_from_private_data(void *private_data)
{
	struct miscdevice *miscdev;

	if (!private_data)
		return NULL;

	miscdev = private_data;
	return container_of(miscdev, lpf_chrdev_t, miscdev);
}

static void lpf_chrdev_fill_info(lpf_chrdev_t *chrdev,
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

static int lpf_chrdev_refresh_info_locked(lpf_chrdev_t *chrdev)
{
	lpf_device_info_t info;
	int32_t ret;

	if (!chrdev)
		return -EINVAL;
	if (chrdev->info.type == LPF_DEVICE_TYPE_INVALID)
		return -ENODEV;

	ret = lpf_device_get_info(chrdev->info.type, chrdev->info.index, &info);
	if (ret != OSAL_SUCCESS)
		return -ENODEV;

	chrdev->info = info;
	osal_atomic_store(&chrdev->error_count, info.error_count);
	return 0;
}

int lpf_chrdev_open(struct file *file)
{
	lpf_chrdev_file_ctx_t *ctx;
	lpf_device_handle_t *handle = NULL;
	lpf_chrdev_t *chrdev;
	int open_count;

	if (!file)
		return -EINVAL;

	chrdev = lpf_chrdev_from_private_data(file->private_data);
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
EXPORT_SYMBOL_GPL(lpf_chrdev_open);

int lpf_chrdev_release(struct file *file)
{
	lpf_chrdev_file_ctx_t *ctx;
	lpf_device_handle_t *handle = NULL;
	lpf_chrdev_t *chrdev;
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
EXPORT_SYMBOL_GPL(lpf_chrdev_release);

int lpf_chrdev_register(lpf_chrdev_t *chrdev, const char *name,
			const struct file_operations *fops)
{
	return lpf_chrdev_register_instance(chrdev, name, NULL, 0, fops);
}
EXPORT_SYMBOL_GPL(lpf_chrdev_register);

int lpf_chrdev_register_instance(lpf_chrdev_t *chrdev, const char *name,
				 const char *nodename, uint32_t index,
				 const struct file_operations *fops)
{
	lpf_device_t device;

	osal_memset(&device, 0, sizeof(device));
	device.config.index = index;
	device.config.name = name;
	osal_strncpy(device.name, name, sizeof(device.name) - 1U);
	device.name[sizeof(device.name) - 1U] = '\0';

	return lpf_chrdev_register_lpf_device(chrdev, name, nodename, &device,
					      fops);
}
EXPORT_SYMBOL_GPL(lpf_chrdev_register_instance);

int lpf_chrdev_register_lpf_device(lpf_chrdev_t *chrdev, const char *name,
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
	lpf_chrdev_fill_info(chrdev, device, index);
	osal_atomic_init(&chrdev->open_count, 0);
	osal_atomic_init(&chrdev->error_count, 0);

	chrdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	chrdev->miscdev.name = chrdev->name;
	chrdev->miscdev.fops = fops;
	chrdev->miscdev.nodename = nodename ? chrdev->nodename : NULL;
	chrdev->miscdev.groups = lpf_chrdev_sysfs_groups();
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
EXPORT_SYMBOL_GPL(lpf_chrdev_register_lpf_device);

void lpf_chrdev_unregister(lpf_chrdev_t *chrdev)
{
	if (!chrdev)
		return;
	if (!chrdev->registered)
		return;

	misc_deregister(&chrdev->miscdev);
	osal_mutex_destroy(&chrdev->lock);
	osal_memset(chrdev, 0, sizeof(*chrdev));
}
EXPORT_SYMBOL_GPL(lpf_chrdev_unregister);

lpf_chrdev_t *lpf_chrdev_from_file(struct file *file)
{
	lpf_chrdev_file_ctx_t *ctx;
	lpf_chrdev_t *chrdev;

	if (!file || !file->private_data)
		return NULL;

	ctx = file->private_data;
	chrdev = ctx->chrdev;
	return chrdev && chrdev->registered ? chrdev : NULL;
}
EXPORT_SYMBOL_GPL(lpf_chrdev_from_file);

int lpf_chrdev_get_info(lpf_chrdev_t *chrdev, lpf_device_info_t *info)
{
	int ret;

	if (!chrdev || !info)
		return -EINVAL;
	if (!chrdev->registered)
		return -ENODEV;

	osal_mutex_lock(&chrdev->lock);
	if (chrdev->registered)
		(void)lpf_chrdev_refresh_info_locked(chrdev);
	*info = chrdev->info;
	ret = chrdev->registered ? 0 : -ENODEV;
	osal_mutex_unlock(&chrdev->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(lpf_chrdev_get_info);

uint32_t lpf_chrdev_open_count(const lpf_chrdev_t *chrdev)
{
	if (!chrdev)
		return 0;

	return osal_atomic_load(&chrdev->open_count);
}
EXPORT_SYMBOL_GPL(lpf_chrdev_open_count);

uint32_t lpf_chrdev_error_count(const lpf_chrdev_t *chrdev)
{
	if (!chrdev)
		return 0;

	return osal_atomic_load(&chrdev->error_count);
}
EXPORT_SYMBOL_GPL(lpf_chrdev_error_count);

void lpf_chrdev_record_error(lpf_chrdev_t *chrdev, int error)
{
	lpf_device_type_t type;
	uint32_t index;

	if (!chrdev || error == 0)
		return;
	if (!chrdev->registered)
		return;

	osal_mutex_lock(&chrdev->lock);
	type = chrdev->info.type;
	index = chrdev->info.index;
	if (type == LPF_DEVICE_TYPE_INVALID) {
		chrdev->info.state = LPF_DEVICE_STATE_ERROR;
		chrdev->info.last_error = error;
		chrdev->info.error_count = osal_atomic_inc(&chrdev->error_count);
	}
	osal_mutex_unlock(&chrdev->lock);

	if (type != LPF_DEVICE_TYPE_INVALID) {
		lpf_device_record_error(type, index, error);
		osal_mutex_lock(&chrdev->lock);
		(void)lpf_chrdev_refresh_info_locked(chrdev);
		osal_mutex_unlock(&chrdev->lock);
	}
}
EXPORT_SYMBOL_GPL(lpf_chrdev_record_error);

uint32_t lpf_chrdev_index(const lpf_chrdev_t *chrdev)
{
	if (!chrdev)
		return 0;

	return chrdev->index;
}
EXPORT_SYMBOL_GPL(lpf_chrdev_index);
