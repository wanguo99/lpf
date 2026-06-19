// SPDX-License-Identifier: GPL-2.0

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "pdi/pdi_mcu.h"
#include "pdm.h"
#include "pdm_mcu_internal.h"

static osal_mutex_t g_pdm_mcu_chrdev_lock;
static osal_atomic_uint32_t g_pdm_mcu_open_count;
static struct miscdevice g_pdm_mcu_miscdev;

static long pdm_mcu_status_to_errno(int32_t status)
{
	if (status == OSAL_SUCCESS)
		return 0;

	if (status == OSAL_ERR_INVALID_PARAM ||
	    status == OSAL_ERR_INVALID_POINTER ||
	    status == OSAL_ERR_INVALID_SIZE ||
	    status == OSAL_ERR_INVALID_ID)
		return -EINVAL;

	if (status == OSAL_ERR_NO_MEMORY)
		return -ENOMEM;

	if (status == OSAL_ERR_TIMEOUT)
		return -ETIMEDOUT;

	if (status == OSAL_ERR_BUSY)
		return -EBUSY;

	if (status == OSAL_ERR_NOT_SUPPORTED)
		return -EOPNOTSUPP;

	if (status == OSAL_ERR_NOT_IMPLEMENTED)
		return -ENOSYS;

	return -EIO;
}

static void pdm_mcu_fill_info(struct pdi_mcu_info *info)
{
	osal_memset(info, 0, sizeof(*info));
	info->abi_version = PDI_MCU_ABI_VERSION;
	info->module_version_major = PDM_VERSION_MAJOR;
	info->module_version_minor = PDM_VERSION_MINOR;
	info->module_version_patch = PDM_VERSION_PATCH;
	info->open_count = osal_atomic_load(&g_pdm_mcu_open_count);
	info->max_devices = CONFIG_PDM_MCU_MAX_DEVICES;
}

static pdm_mcu_handle_t pdm_mcu_open_index(u32 index)
{
	if (index >= CONFIG_PDM_MCU_MAX_DEVICES)
		return NULL;

	return pdm_mcu_get(index);
}

static long pdm_mcu_ioctl_get_info(unsigned long arg)
{
	struct pdi_mcu_info info;

	osal_mutex_lock(&g_pdm_mcu_chrdev_lock);
	pdm_mcu_fill_info(&info);
	osal_mutex_unlock(&g_pdm_mcu_chrdev_lock);

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
		return pdm_mcu_status_to_errno(ret);

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
		return pdm_mcu_status_to_errno(ret);

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
	return pdm_mcu_status_to_errno(ret);
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
		return pdm_mcu_status_to_errno(ret);

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
		return pdm_mcu_status_to_errno(ret);

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
	return pdm_mcu_status_to_errno(ret);
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
	int open_count;

	(void)inode;
	(void)file;

	osal_mutex_lock(&g_pdm_mcu_chrdev_lock);
	open_count = (int)osal_atomic_inc(&g_pdm_mcu_open_count);
	osal_mutex_unlock(&g_pdm_mcu_chrdev_lock);

	LOG_INFO(PDI_MCU_DEVICE_NAME, "open count=%d", open_count);
	return 0;
}

static int pdm_mcu_release(struct inode *inode, struct file *file)
{
	int open_count;

	(void)inode;
	(void)file;

	osal_mutex_lock(&g_pdm_mcu_chrdev_lock);
	if (osal_atomic_load(&g_pdm_mcu_open_count) > 0)
		osal_atomic_dec(&g_pdm_mcu_open_count);
	open_count = (int)osal_atomic_load(&g_pdm_mcu_open_count);
	osal_mutex_unlock(&g_pdm_mcu_chrdev_lock);

	LOG_INFO(PDI_MCU_DEVICE_NAME, "release count=%d", open_count);
	return 0;
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
	int ret;

	ret = osal_mutex_init(&g_pdm_mcu_chrdev_lock, NULL);
	if (ret != OSAL_SUCCESS)
		return -ret;

	osal_atomic_init(&g_pdm_mcu_open_count, 0);

	g_pdm_mcu_miscdev.minor = MISC_DYNAMIC_MINOR;
	g_pdm_mcu_miscdev.name = PDI_MCU_DEVICE_NAME;
	g_pdm_mcu_miscdev.fops = &pdm_mcu_fops;
	g_pdm_mcu_miscdev.mode = 0666;

	ret = misc_register(&g_pdm_mcu_miscdev);
	if (ret) {
		osal_mutex_destroy(&g_pdm_mcu_chrdev_lock);
		return ret;
	}

	LOG_INFO(PDI_MCU_DEVICE_NAME, "/dev/%s ready", PDI_MCU_DEVICE_NAME);
	return 0;
}

void pdm_mcu_chrdev_unregister(void)
{
	misc_deregister(&g_pdm_mcu_miscdev);
	osal_mutex_destroy(&g_pdm_mcu_chrdev_lock);
}
