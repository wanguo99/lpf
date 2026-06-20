// SPDX-License-Identifier: GPL-2.0

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/module.h>

#include "lpf/lpf_mcu.h"
#include "lpf/lpf_chrdev.h"
#include "lpf/lpf_errno.h"
#include "lpf_mcu_internal.h"

static lpf_chrdev_t g_lpf_mcu_chrdevs[LPF_MCU_MAX_DEVICES];

static lpf_chrdev_t *lpf_mcu_chrdev_from_file(struct file *file)
{
	return lpf_chrdev_from_file(file);
}

static uint32_t lpf_mcu_file_index(struct file *file)
{
	lpf_chrdev_t *chrdev = lpf_mcu_chrdev_from_file(file);

	return lpf_chrdev_index(chrdev);
}

static void lpf_mcu_record_file_error(struct file *file, long ret)
{
	if (lpf_errno_is_runtime_error(ret))
		lpf_chrdev_record_error(lpf_mcu_chrdev_from_file(file), (int)ret);
}

static uint32_t lpf_mcu_open_count(void)
{
	uint32_t open_count = 0;
	uint32_t i;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_lpf_mcu_chrdevs); i++)
		open_count += lpf_chrdev_open_count(&g_lpf_mcu_chrdevs[i]);

	return open_count;
}

static void lpf_mcu_fill_info(struct lpf_mcu_info *info)
{
	osal_memset(info, 0, sizeof(*info));
	info->abi_version = LPF_MCU_ABI_VERSION;
	info->module_version_major = LPF_MCU_SERVICE_VERSION_MAJOR;
	info->module_version_minor = LPF_MCU_SERVICE_VERSION_MINOR;
	info->module_version_patch = LPF_MCU_SERVICE_VERSION_PATCH;
	info->open_count = lpf_mcu_open_count();
	info->max_devices = LPF_MCU_MAX_DEVICES;
}

static lpf_mcu_handle_t lpf_mcu_open_index(u32 index)
{
	if (index >= LPF_MCU_MAX_DEVICES)
		return NULL;

	return lpf_mcu_get(index);
}

static long lpf_mcu_ioctl_get_info(unsigned long arg)
{
	struct lpf_mcu_info info;
	int32_t ret;

	lpf_mcu_fill_info(&info);

	ret = osal_copy_to_user((void __user *)arg, &info, sizeof(info));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	return 0;
}

static long lpf_mcu_ioctl_get_version(struct file *file, unsigned long arg)
{
	struct lpf_mcu_version request;
	lpf_mcu_version_t version;
	lpf_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	request.index = lpf_mcu_file_index(file);
	handle = lpf_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	osal_memset(&version, 0, sizeof(version));
	ret = lpf_mcu_get_version(handle, &version);
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	request.major = version.major;
	request.minor = version.minor;
	request.patch = version.patch;
	request.build = version.build;
	osal_strncpy(request.version_string, version.version_string,
		     sizeof(request.version_string));

	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	return 0;
}

static long lpf_mcu_ioctl_get_status(struct file *file, unsigned long arg)
{
	struct lpf_mcu_status request;
	lpf_mcu_status_t status;
	lpf_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	request.index = lpf_mcu_file_index(file);
	handle = lpf_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	osal_memset(&status, 0, sizeof(status));
	ret = lpf_mcu_get_status(handle, &status);
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	request.online = status.online ? 1U : 0U;
	request.state = status.state;
	request.uptime_sec = status.uptime_sec;
	request.error_code = status.error_code;
	request.temperature_milli_celsius = status.temperature_milli_celsius;
	request.voltage_mv = status.voltage_mv;
	request.timestamp_us = status.timestamp_us;

	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	return 0;
}

static long lpf_mcu_ioctl_reset(struct file *file, unsigned long arg)
{
	u32 index;
	lpf_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&index, (void __user *)arg, sizeof(index));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	index = lpf_mcu_file_index(file);
	handle = lpf_mcu_open_index(index);
	if (!handle)
		return -ENODEV;

	ret = lpf_mcu_reset(handle);
	return lpf_status_to_errno(ret);
}

static long lpf_mcu_ioctl_command(struct file *file, unsigned long arg)
{
	struct lpf_mcu_command request;
	lpf_mcu_handle_t handle;
	uint32_t actual_len = 0;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	if (request.tx_len > sizeof(request.tx_data) ||
	    request.rx_len > sizeof(request.rx_data))
		return -EINVAL;
	if (request.command > 0xFFU)
		return -EINVAL;

	request.index = lpf_mcu_file_index(file);
	handle = lpf_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = lpf_mcu_send_command(handle, (uint8_t)request.command,
				   request.tx_data, request.tx_len,
				   request.rx_data, request.rx_len, &actual_len);
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	request.rx_len = actual_len;
	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	return 0;
}

static long lpf_mcu_ioctl_read_data(struct file *file, unsigned long arg)
{
	struct lpf_mcu_data request;
	lpf_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	if (request.len > sizeof(request.data))
		return -EINVAL;

	request.index = lpf_mcu_file_index(file);
	handle = lpf_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = lpf_mcu_read_data(handle, request.address, request.data,
				request.len);
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	return 0;
}

static long lpf_mcu_ioctl_write_data(struct file *file, unsigned long arg)
{
	struct lpf_mcu_data request;
	lpf_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	if (request.len > LPF_MCU_MAX_WRITE_SIZE)
		return -EINVAL;

	request.index = lpf_mcu_file_index(file);
	handle = lpf_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = lpf_mcu_write_data(handle, request.address, request.data,
				 request.len);
	return lpf_status_to_errno(ret);
}

static long lpf_mcu_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	long ret;

	(void)file;

	switch (cmd) {
	case LPF_MCU_IOC_GET_INFO:
		ret = lpf_mcu_ioctl_get_info(arg);
		break;
	case LPF_MCU_IOC_GET_VERSION:
		ret = lpf_mcu_ioctl_get_version(file, arg);
		break;
	case LPF_MCU_IOC_GET_STATUS:
		ret = lpf_mcu_ioctl_get_status(file, arg);
		break;
	case LPF_MCU_IOC_RESET:
		ret = lpf_mcu_ioctl_reset(file, arg);
		break;
	case LPF_MCU_IOC_COMMAND:
		ret = lpf_mcu_ioctl_command(file, arg);
		break;
	case LPF_MCU_IOC_READ_DATA:
		ret = lpf_mcu_ioctl_read_data(file, arg);
		break;
	case LPF_MCU_IOC_WRITE_DATA:
		ret = lpf_mcu_ioctl_write_data(file, arg);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	lpf_mcu_record_file_error(file, ret);
	return ret;
}

#ifdef CONFIG_COMPAT
static long lpf_mcu_compat_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	return lpf_mcu_ioctl(file, cmd, arg);
}
#endif

static int lpf_mcu_open(struct inode *inode, struct file *file)
{
	(void)inode;

	return lpf_chrdev_open(file);
}

static int lpf_mcu_release(struct inode *inode, struct file *file)
{
	(void)inode;

	return lpf_chrdev_release(file);
}

static const struct file_operations lpf_mcu_fops = {
	.owner = THIS_MODULE,
	.open = lpf_mcu_open,
	.release = lpf_mcu_release,
	.unlocked_ioctl = lpf_mcu_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = lpf_mcu_compat_ioctl,
#endif
	.llseek = noop_llseek,
};

int lpf_mcu_chrdev_register(void)
{
	return 0;
}

void lpf_mcu_chrdev_unregister(void)
{
	uint32_t i;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_lpf_mcu_chrdevs); i++)
		lpf_chrdev_unregister(&g_lpf_mcu_chrdevs[i]);
}

int lpf_mcu_chrdev_register_device(const lpf_device_t *device)
{
	char name[LPF_CHRDEV_NAME_LEN];
	char nodename[LPF_CHRDEV_NAME_LEN];
	uint32_t index;

	if (!device || device->config.type != LPF_DEVICE_TYPE_MCU)
		return -EINVAL;

	index = device->config.index;
	if (index >= OSAL_ARRAY_SIZE(g_lpf_mcu_chrdevs))
		return -EINVAL;

	osal_snprintf(name, sizeof(name), "lpf_mcu%u", index);
	osal_snprintf(nodename, sizeof(nodename), "lpf/mcu%u", index);

	return lpf_chrdev_register_lpf_device(&g_lpf_mcu_chrdevs[index],
					      name, nodename, device,
					      &lpf_mcu_fops);
}

void lpf_mcu_chrdev_unregister_device(const lpf_device_t *device)
{
	uint32_t index;

	if (!device || device->config.type != LPF_DEVICE_TYPE_MCU)
		return;

	index = device->config.index;
	if (index >= OSAL_ARRAY_SIZE(g_lpf_mcu_chrdevs))
		return;

	lpf_chrdev_unregister(&g_lpf_mcu_chrdevs[index]);
}

void lpf_mcu_chrdev_record_error(uint32_t index, int error)
{
	if (index >= OSAL_ARRAY_SIZE(g_lpf_mcu_chrdevs))
		return;

	lpf_chrdev_record_error(&g_lpf_mcu_chrdevs[index], error);
}
