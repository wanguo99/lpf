// SPDX-License-Identifier: GPL-2.0

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "pdi/pdi_mcu.h"
#include "pdm.h"
#include "base/pdm_chrdev.h"
#include "pdm_mcu_internal.h"

static pdm_chrdev_t g_pdm_mcu_chrdev;

static void pdm_mcu_fill_info(struct pdi_mcu_info *info)
{
	osal_memset(info, 0, sizeof(*info));
	info->abi_version = PDI_MCU_ABI_VERSION;
	info->module_version_major = PDM_VERSION_MAJOR;
	info->module_version_minor = PDM_VERSION_MINOR;
	info->module_version_patch = PDM_VERSION_PATCH;
	info->open_count = pdm_chrdev_open_count(&g_pdm_mcu_chrdev);
	info->max_devices = PDM_MCU_MAX_DEVICES;
}

static pdm_mcu_handle_t pdm_mcu_open_index(u32 index)
{
	if (index >= PDM_MCU_MAX_DEVICES)
		return NULL;

	return pdm_mcu_get(index);
}

static long pdm_mcu_ioctl_get_info(unsigned long arg)
{
	struct pdi_mcu_info info;

	pdm_mcu_fill_info(&info);

	if (copy_to_user((void __user *)arg, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static long pdm_mcu_ioctl_get_version(unsigned long arg)
{
	struct pdi_mcu_version request;
	pdm_mcu_version_t version;
	pdm_mcu_handle_t handle;
	int32_t ret;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;

	handle = pdm_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	osal_memset(&version, 0, sizeof(version));
	ret = pdm_mcu_get_version(handle, &version);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.major = version.major;
	request.minor = version.minor;
	request.patch = version.patch;
	request.build = version.build;
	osal_strncpy(request.version_string, version.version_string,
		     sizeof(request.version_string));

	if (copy_to_user((void __user *)arg, &request, sizeof(request)))
		return -EFAULT;

	return 0;
}

static long pdm_mcu_ioctl_get_status(unsigned long arg)
{
	struct pdi_mcu_status request;
	pdm_mcu_status_t status;
	pdm_mcu_handle_t handle;
	int32_t ret;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;

	handle = pdm_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	osal_memset(&status, 0, sizeof(status));
	ret = pdm_mcu_get_status(handle, &status);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.online = status.online ? 1U : 0U;
	request.state = status.state;
	request.uptime_sec = status.uptime_sec;
	request.error_code = status.error_code;
	request.temperature_milli_celsius = status.temperature_milli_celsius;
	request.voltage_mv = status.voltage_mv;
	request.timestamp_us = status.timestamp_us;

	if (copy_to_user((void __user *)arg, &request, sizeof(request)))
		return -EFAULT;

	return 0;
}

static long pdm_mcu_ioctl_reset(unsigned long arg)
{
	u32 index;
	pdm_mcu_handle_t handle;
	int32_t ret;

	if (copy_from_user(&index, (void __user *)arg, sizeof(index)))
		return -EFAULT;

	handle = pdm_mcu_open_index(index);
	if (!handle)
		return -ENODEV;

	ret = pdm_mcu_reset(handle);
	return pdm_status_to_errno(ret);
}

static long pdm_mcu_ioctl_command(unsigned long arg)
{
	struct pdi_mcu_command request;
	pdm_mcu_handle_t handle;
	uint32_t actual_len = 0;
	int32_t ret;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;

	if (request.tx_len > sizeof(request.tx_data) ||
	    request.rx_len > sizeof(request.rx_data))
		return -EINVAL;
	if (request.command > 0xFFU)
		return -EINVAL;

	handle = pdm_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_mcu_send_command(handle, (uint8_t)request.command,
				   request.tx_data, request.tx_len,
				   request.rx_data, request.rx_len, &actual_len);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.rx_len = actual_len;
	if (copy_to_user((void __user *)arg, &request, sizeof(request)))
		return -EFAULT;

	return 0;
}

static long pdm_mcu_ioctl_read_data(unsigned long arg)
{
	struct pdi_mcu_data request;
	pdm_mcu_handle_t handle;
	int32_t ret;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;

	if (request.len > sizeof(request.data))
		return -EINVAL;

	handle = pdm_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_mcu_read_data(handle, request.address, request.data,
				request.len);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	if (copy_to_user((void __user *)arg, &request, sizeof(request)))
		return -EFAULT;

	return 0;
}

static long pdm_mcu_ioctl_write_data(unsigned long arg)
{
	struct pdi_mcu_data request;
	pdm_mcu_handle_t handle;
	int32_t ret;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;

	if (request.len > PDI_MCU_MAX_WRITE_SIZE)
		return -EINVAL;

	handle = pdm_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_mcu_write_data(handle, request.address, request.data,
				 request.len);
	return pdm_status_to_errno(ret);
}

static long pdm_mcu_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	(void)file;

	switch (cmd) {
	case PDI_MCU_IOC_GET_INFO:
		return pdm_mcu_ioctl_get_info(arg);
	case PDI_MCU_IOC_GET_VERSION:
		return pdm_mcu_ioctl_get_version(arg);
	case PDI_MCU_IOC_GET_STATUS:
		return pdm_mcu_ioctl_get_status(arg);
	case PDI_MCU_IOC_RESET:
		return pdm_mcu_ioctl_reset(arg);
	case PDI_MCU_IOC_COMMAND:
		return pdm_mcu_ioctl_command(arg);
	case PDI_MCU_IOC_READ_DATA:
		return pdm_mcu_ioctl_read_data(arg);
	case PDI_MCU_IOC_WRITE_DATA:
		return pdm_mcu_ioctl_write_data(arg);
	default:
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
static long pdm_mcu_compat_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	return pdm_mcu_ioctl(file, cmd, arg);
}
#endif

static int pdm_mcu_open(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	return pdm_chrdev_open(&g_pdm_mcu_chrdev);
}

static int pdm_mcu_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	return pdm_chrdev_release(&g_pdm_mcu_chrdev);
}

static const struct file_operations pdm_mcu_fops = {
	.owner = THIS_MODULE,
	.open = pdm_mcu_open,
	.release = pdm_mcu_release,
	.unlocked_ioctl = pdm_mcu_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pdm_mcu_compat_ioctl,
#endif
	.llseek = noop_llseek,
};

int pdm_mcu_chrdev_register(void)
{
	return pdm_chrdev_register(&g_pdm_mcu_chrdev, PDI_MCU_DEVICE_NAME,
				   &pdm_mcu_fops);
}

void pdm_mcu_chrdev_unregister(void)
{
	pdm_chrdev_unregister(&g_pdm_mcu_chrdev);
}
