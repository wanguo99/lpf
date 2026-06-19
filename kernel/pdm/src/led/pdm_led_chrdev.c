// SPDX-License-Identifier: GPL-2.0

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "pdi/pdi_led.h"
#include "pdm.h"
#include "base/pdm_chrdev.h"
#include "pdm_led_internal.h"

static pdm_chrdev_t g_pdm_led_chrdev;

static void pdm_led_fill_info(struct pdi_led_info *info)
{
	osal_memset(info, 0, sizeof(*info));
	info->abi_version = PDI_LED_ABI_VERSION;
	info->module_version_major = PDM_LED_VERSION_MAJOR;
	info->module_version_minor = PDM_LED_VERSION_MINOR;
	info->module_version_patch = PDM_LED_VERSION_PATCH;
	info->open_count = pdm_chrdev_open_count(&g_pdm_led_chrdev);
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

	pdm_led_fill_info(&info);

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
		return pdm_status_to_errno(ret);

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
	return pdm_status_to_errno(ret);
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
	return pdm_status_to_errno(ret);
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
	return pdm_status_to_errno(ret);
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
	(void)inode;
	(void)file;

	return pdm_chrdev_open(&g_pdm_led_chrdev);
}

static int pdm_led_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	return pdm_chrdev_release(&g_pdm_led_chrdev);
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
	return pdm_chrdev_register(&g_pdm_led_chrdev, PDI_LED_DEVICE_NAME,
				   &pdm_led_fops);
}

void pdm_led_chrdev_unregister(void)
{
	pdm_chrdev_unregister(&g_pdm_led_chrdev);
}
