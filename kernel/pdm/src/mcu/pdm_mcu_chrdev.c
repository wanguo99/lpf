// SPDX-License-Identifier: GPL-2.0

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/module.h>

#include "lpf/lpf_mcu.h"
#include "pdm.h"
#include "pdm_chrdev.h"
#include "pdm_mcu_internal.h"
#include "pdm_status.h"

static pdm_chrdev_t g_pdm_mcu_chrdevs[PDM_MCU_MAX_DEVICES];

static pdm_chrdev_t *pdm_mcu_chrdev_from_file(struct file *file)
{
	return pdm_chrdev_from_file(file);
}

static uint32_t pdm_mcu_file_index(struct file *file)
{
	pdm_chrdev_t *chrdev = pdm_mcu_chrdev_from_file(file);

	return pdm_chrdev_index(chrdev);
}

static void pdm_mcu_record_file_error(struct file *file, long ret)
{
	if (ret < 0)
		pdm_chrdev_record_error(pdm_mcu_chrdev_from_file(file), (int)ret);
}

static uint32_t pdm_mcu_open_count(void)
{
	uint32_t open_count = 0;
	uint32_t i;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_mcu_chrdevs); i++)
		open_count += pdm_chrdev_open_count(&g_pdm_mcu_chrdevs[i]);

	return open_count;
}

static void pdm_mcu_fill_info(struct lpf_mcu_info *info)
{
	osal_memset(info, 0, sizeof(*info));
	info->abi_version = LPF_MCU_ABI_VERSION;
	info->module_version_major = PDM_VERSION_MAJOR;
	info->module_version_minor = PDM_VERSION_MINOR;
	info->module_version_patch = PDM_VERSION_PATCH;
	info->open_count = pdm_mcu_open_count();
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
	struct lpf_mcu_info info;
	int32_t ret;

	pdm_mcu_fill_info(&info);

	ret = osal_copy_to_user((void __user *)arg, &info, sizeof(info));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	return 0;
}

static long pdm_mcu_ioctl_get_version(struct file *file, unsigned long arg)
{
	struct lpf_mcu_version request;
	pdm_mcu_version_t version;
	pdm_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.index = pdm_mcu_file_index(file);
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

	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	return 0;
}

static long pdm_mcu_ioctl_get_status(struct file *file, unsigned long arg)
{
	struct lpf_mcu_status request;
	pdm_mcu_status_t status;
	pdm_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.index = pdm_mcu_file_index(file);
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

	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	return 0;
}

static long pdm_mcu_ioctl_reset(struct file *file, unsigned long arg)
{
	u32 index;
	pdm_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&index, (void __user *)arg, sizeof(index));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	index = pdm_mcu_file_index(file);
	handle = pdm_mcu_open_index(index);
	if (!handle)
		return -ENODEV;

	ret = pdm_mcu_reset(handle);
	return pdm_status_to_errno(ret);
}

static long pdm_mcu_ioctl_command(struct file *file, unsigned long arg)
{
	struct lpf_mcu_command request;
	pdm_mcu_handle_t handle;
	uint32_t actual_len = 0;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	if (request.tx_len > sizeof(request.tx_data) ||
	    request.rx_len > sizeof(request.rx_data))
		return -EINVAL;
	if (request.command > 0xFFU)
		return -EINVAL;

	request.index = pdm_mcu_file_index(file);
	handle = pdm_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_mcu_send_command(handle, (uint8_t)request.command,
				   request.tx_data, request.tx_len,
				   request.rx_data, request.rx_len, &actual_len);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.rx_len = actual_len;
	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	return 0;
}

static long pdm_mcu_ioctl_read_data(struct file *file, unsigned long arg)
{
	struct lpf_mcu_data request;
	pdm_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	if (request.len > sizeof(request.data))
		return -EINVAL;

	request.index = pdm_mcu_file_index(file);
	handle = pdm_mcu_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_mcu_read_data(handle, request.address, request.data,
				request.len);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	return 0;
}

static long pdm_mcu_ioctl_write_data(struct file *file, unsigned long arg)
{
	struct lpf_mcu_data request;
	pdm_mcu_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	if (request.len > LPF_MCU_MAX_WRITE_SIZE)
		return -EINVAL;

	request.index = pdm_mcu_file_index(file);
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
	long ret;

	(void)file;

	switch (cmd) {
	case LPF_MCU_IOC_GET_INFO:
		ret = pdm_mcu_ioctl_get_info(arg);
		break;
	case LPF_MCU_IOC_GET_VERSION:
		ret = pdm_mcu_ioctl_get_version(file, arg);
		break;
	case LPF_MCU_IOC_GET_STATUS:
		ret = pdm_mcu_ioctl_get_status(file, arg);
		break;
	case LPF_MCU_IOC_RESET:
		ret = pdm_mcu_ioctl_reset(file, arg);
		break;
	case LPF_MCU_IOC_COMMAND:
		ret = pdm_mcu_ioctl_command(file, arg);
		break;
	case LPF_MCU_IOC_READ_DATA:
		ret = pdm_mcu_ioctl_read_data(file, arg);
		break;
	case LPF_MCU_IOC_WRITE_DATA:
		ret = pdm_mcu_ioctl_write_data(file, arg);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	pdm_mcu_record_file_error(file, ret);
	return ret;
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

	return pdm_chrdev_open(file);
}

static int pdm_mcu_release(struct inode *inode, struct file *file)
{
	(void)inode;

	return pdm_chrdev_release(file);
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
	return 0;
}

void pdm_mcu_chrdev_unregister(void)
{
	uint32_t i;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_mcu_chrdevs); i++)
		pdm_chrdev_unregister(&g_pdm_mcu_chrdevs[i]);
}

int pdm_mcu_chrdev_register_device(const lpf_device_t *device)
{
	char name[PDM_CHRDEV_NAME_LEN];
	char nodename[PDM_CHRDEV_NAME_LEN];
	uint32_t index;

	if (!device || device->config.type != LPF_DEVICE_TYPE_MCU)
		return -EINVAL;

	index = device->config.index;
	if (index >= OSAL_ARRAY_SIZE(g_pdm_mcu_chrdevs))
		return -EINVAL;

	osal_snprintf(name, sizeof(name), "lpf_mcu%u", index);
	osal_snprintf(nodename, sizeof(nodename), "lpf/mcu%u", index);

	return pdm_chrdev_register_lpf_device(&g_pdm_mcu_chrdevs[index],
					      name, nodename, device,
					      &pdm_mcu_fops);
}

void pdm_mcu_chrdev_unregister_device(const lpf_device_t *device)
{
	uint32_t index;

	if (!device || device->config.type != LPF_DEVICE_TYPE_MCU)
		return;

	index = device->config.index;
	if (index >= OSAL_ARRAY_SIZE(g_pdm_mcu_chrdevs))
		return;

	pdm_chrdev_unregister(&g_pdm_mcu_chrdevs[index]);
}

void pdm_mcu_chrdev_record_error(uint32_t index, int error)
{
	if (index >= OSAL_ARRAY_SIZE(g_pdm_mcu_chrdevs))
		return;

	pdm_chrdev_record_error(&g_pdm_mcu_chrdevs[index], error);
}
