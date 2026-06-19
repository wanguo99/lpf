// SPDX-License-Identifier: GPL-2.0

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "pdi/pdi_led.h"
#include "pdm.h"
#include "pdm_led_internal.h"

static osal_mutex_t g_pdm_led_chrdev_lock;
static osal_atomic_uint32_t g_pdm_led_open_count;
static struct miscdevice g_pdm_led_miscdev;

static long pdm_led_status_to_errno(int32_t status)
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

static void pdm_led_fill_info(struct pdi_led_info *info)
{
	osal_memset(info, 0, sizeof(*info));
	info->abi_version = PDI_LED_ABI_VERSION;
	info->module_version_major = PDM_LED_VERSION_MAJOR;
	info->module_version_minor = PDM_LED_VERSION_MINOR;
	info->module_version_patch = PDM_LED_VERSION_PATCH;
	info->open_count = osal_atomic_load(&g_pdm_led_open_count);
	info->max_devices = PDM_LED_MAX_DEVICES;
}

static pdm_led_handle_t pdm_led_open_index(u32 index)
{
	if (index >= PDM_LED_MAX_DEVICES)
		return NULL;

	return pdm_led_get(index);
}

static long pdm_led_ioctl_get_info(unsigned long arg)
{
	struct pdi_led_info info;

	osal_mutex_lock(&g_pdm_led_chrdev_lock);
	pdm_led_fill_info(&info);
	osal_mutex_unlock(&g_pdm_led_chrdev_lock);

	if (copy_to_user((void __user *)arg, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static long pdm_led_ioctl_get_state(unsigned long arg)
{
	struct pdi_led_state request;
	pdm_led_state_t state;
	pdm_led_handle_t handle;
	int32_t ret;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;

	handle = pdm_led_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_led_get_state(handle, &state);
	if (ret != OSAL_SUCCESS)
		return pdm_led_status_to_errno(ret);

	request.brightness = state.brightness;
	request.max_brightness = state.max_brightness;
	request.enabled = state.enabled ? 1U : 0U;

	if (copy_to_user((void __user *)arg, &request, sizeof(request)))
		return -EFAULT;

	return 0;
}

static long pdm_led_ioctl_set_brightness(unsigned long arg)
{
	struct pdi_led_brightness request;
	pdm_led_handle_t handle;
	int32_t ret;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;

	handle = pdm_led_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_led_set_brightness(handle, request.brightness);
	return pdm_led_status_to_errno(ret);
}

static long pdm_led_ioctl_enable(unsigned long arg)
{
	u32 index;
	pdm_led_handle_t handle;
	int32_t ret;

	if (copy_from_user(&index, (void __user *)arg, sizeof(index)))
		return -EFAULT;

	handle = pdm_led_open_index(index);
	if (!handle)
		return -ENODEV;

	ret = pdm_led_enable(handle);
	return pdm_led_status_to_errno(ret);
}

static long pdm_led_ioctl_disable(unsigned long arg)
{
	u32 index;
	pdm_led_handle_t handle;
	int32_t ret;

	if (copy_from_user(&index, (void __user *)arg, sizeof(index)))
		return -EFAULT;

	handle = pdm_led_open_index(index);
	if (!handle)
		return -ENODEV;

	ret = pdm_led_disable(handle);
	return pdm_led_status_to_errno(ret);
}

static long pdm_led_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	(void)file;

	switch (cmd) {
	case PDI_LED_IOC_GET_INFO:
		return pdm_led_ioctl_get_info(arg);
	case PDI_LED_IOC_GET_STATE:
		return pdm_led_ioctl_get_state(arg);
	case PDI_LED_IOC_SET_BRIGHTNESS:
		return pdm_led_ioctl_set_brightness(arg);
	case PDI_LED_IOC_ENABLE:
		return pdm_led_ioctl_enable(arg);
	case PDI_LED_IOC_DISABLE:
		return pdm_led_ioctl_disable(arg);
	default:
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
static long pdm_led_compat_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	return pdm_led_ioctl(file, cmd, arg);
}
#endif

static int pdm_led_open(struct inode *inode, struct file *file)
{
	int open_count;

	(void)inode;
	(void)file;

	osal_mutex_lock(&g_pdm_led_chrdev_lock);
	open_count = (int)osal_atomic_inc(&g_pdm_led_open_count);
	osal_mutex_unlock(&g_pdm_led_chrdev_lock);

	LOG_INFO(PDI_LED_DEVICE_NAME, "open count=%d", open_count);
	return 0;
}

static int pdm_led_release(struct inode *inode, struct file *file)
{
	int open_count;

	(void)inode;
	(void)file;

	osal_mutex_lock(&g_pdm_led_chrdev_lock);
	if (osal_atomic_load(&g_pdm_led_open_count) > 0)
		osal_atomic_dec(&g_pdm_led_open_count);
	open_count = (int)osal_atomic_load(&g_pdm_led_open_count);
	osal_mutex_unlock(&g_pdm_led_chrdev_lock);

	LOG_INFO(PDI_LED_DEVICE_NAME, "release count=%d", open_count);
	return 0;
}

static const struct file_operations pdm_led_fops = {
	.owner = THIS_MODULE,
	.open = pdm_led_open,
	.release = pdm_led_release,
	.unlocked_ioctl = pdm_led_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pdm_led_compat_ioctl,
#endif
	.llseek = noop_llseek,
};

int pdm_led_chrdev_register(void)
{
	int ret;

	ret = osal_mutex_init(&g_pdm_led_chrdev_lock, NULL);
	if (ret != OSAL_SUCCESS)
		return -ret;

	osal_atomic_init(&g_pdm_led_open_count, 0);

	g_pdm_led_miscdev.minor = MISC_DYNAMIC_MINOR;
	g_pdm_led_miscdev.name = PDI_LED_DEVICE_NAME;
	g_pdm_led_miscdev.fops = &pdm_led_fops;
	g_pdm_led_miscdev.mode = 0666;

	ret = misc_register(&g_pdm_led_miscdev);
	if (ret) {
		osal_mutex_destroy(&g_pdm_led_chrdev_lock);
		return ret;
	}

	LOG_INFO(PDI_LED_DEVICE_NAME, "/dev/%s ready", PDI_LED_DEVICE_NAME);
	return 0;
}

void pdm_led_chrdev_unregister(void)
{
	misc_deregister(&g_pdm_led_miscdev);
	osal_mutex_destroy(&g_pdm_led_chrdev_lock);
}
