// SPDX-License-Identifier: GPL-2.0

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/module.h>

#include "lpf/lpf_led.h"
#include "pdm.h"
#include "pdm_chrdev.h"
#include "pdm_led_internal.h"
#include "pdm_status.h"

static pdm_chrdev_t g_pdm_led_chrdevs[PDM_LED_MAX_DEVICES];

static pdm_chrdev_t *pdm_led_chrdev_from_file(struct file *file)
{
	return pdm_chrdev_from_file(file);
}

static uint32_t pdm_led_file_index(struct file *file)
{
	pdm_chrdev_t *chrdev = pdm_led_chrdev_from_file(file);

	return pdm_chrdev_index(chrdev);
}

static void pdm_led_record_file_error(struct file *file, long ret)
{
	if (ret < 0)
		pdm_chrdev_record_error(pdm_led_chrdev_from_file(file), (int)ret);
}

static uint32_t pdm_led_open_count(void)
{
	uint32_t open_count = 0;
	uint32_t i;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_led_chrdevs); i++)
		open_count += pdm_chrdev_open_count(&g_pdm_led_chrdevs[i]);

	return open_count;
}

static void pdm_led_fill_info(struct lpf_led_info *info)
{
	osal_memset(info, 0, sizeof(*info));
	info->abi_version = LPF_LED_ABI_VERSION;
	info->module_version_major = PDM_LED_VERSION_MAJOR;
	info->module_version_minor = PDM_LED_VERSION_MINOR;
	info->module_version_patch = PDM_LED_VERSION_PATCH;
	info->open_count = pdm_led_open_count();
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
	struct lpf_led_info info;
	int32_t ret;

	pdm_led_fill_info(&info);

	ret = osal_copy_to_user((void __user *)arg, &info, sizeof(info));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	return 0;
}

static long pdm_led_ioctl_get_state(struct file *file, unsigned long arg)
{
	struct lpf_led_state request;
	pdm_led_state_t state;
	pdm_led_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.index = pdm_led_file_index(file);
	handle = pdm_led_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_led_get_state(handle, &state);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.brightness = state.brightness;
	request.max_brightness = state.max_brightness;
	request.enabled = state.enabled ? 1U : 0U;

	ret = osal_copy_to_user((void __user *)arg, &request,
				sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	return 0;
}

static long pdm_led_ioctl_set_brightness(struct file *file, unsigned long arg)
{
	struct lpf_led_brightness request;
	pdm_led_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&request, (void __user *)arg,
				  sizeof(request));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	request.index = pdm_led_file_index(file);
	handle = pdm_led_open_index(request.index);
	if (!handle)
		return -ENODEV;

	ret = pdm_led_set_brightness(handle, request.brightness);
	return pdm_status_to_errno(ret);
}

static long pdm_led_ioctl_enable(struct file *file, unsigned long arg)
{
	u32 index;
	pdm_led_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&index, (void __user *)arg, sizeof(index));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	index = pdm_led_file_index(file);
	handle = pdm_led_open_index(index);
	if (!handle)
		return -ENODEV;

	ret = pdm_led_enable(handle);
	return pdm_status_to_errno(ret);
}

static long pdm_led_ioctl_disable(struct file *file, unsigned long arg)
{
	u32 index;
	pdm_led_handle_t handle;
	int32_t ret;

	ret = osal_copy_from_user(&index, (void __user *)arg, sizeof(index));
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	index = pdm_led_file_index(file);
	handle = pdm_led_open_index(index);
	if (!handle)
		return -ENODEV;

	ret = pdm_led_disable(handle);
	return pdm_status_to_errno(ret);
}

static long pdm_led_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	long ret;

	(void)file;

	switch (cmd) {
	case LPF_LED_IOC_GET_INFO:
		ret = pdm_led_ioctl_get_info(arg);
		break;
	case LPF_LED_IOC_GET_STATE:
		ret = pdm_led_ioctl_get_state(file, arg);
		break;
	case LPF_LED_IOC_SET_BRIGHTNESS:
		ret = pdm_led_ioctl_set_brightness(file, arg);
		break;
	case LPF_LED_IOC_ENABLE:
		ret = pdm_led_ioctl_enable(file, arg);
		break;
	case LPF_LED_IOC_DISABLE:
		ret = pdm_led_ioctl_disable(file, arg);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	pdm_led_record_file_error(file, ret);
	return ret;
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

	return pdm_chrdev_open(file);
}

static int pdm_led_release(struct inode *inode, struct file *file)
{
	(void)inode;

	return pdm_chrdev_release(file);
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
	return 0;
}

void pdm_led_chrdev_unregister(void)
{
	uint32_t i;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_led_chrdevs); i++)
		pdm_chrdev_unregister(&g_pdm_led_chrdevs[i]);
}

int pdm_led_chrdev_register_device(const lpf_device_t *device)
{
	char name[PDM_CHRDEV_NAME_LEN];
	char nodename[PDM_CHRDEV_NAME_LEN];
	uint32_t index;

	if (!device || device->config.type != LPF_DEVICE_TYPE_LED)
		return -EINVAL;

	index = device->config.index;
	if (index >= OSAL_ARRAY_SIZE(g_pdm_led_chrdevs))
		return -EINVAL;

	osal_snprintf(name, sizeof(name), "lpf_led%u", index);
	osal_snprintf(nodename, sizeof(nodename), "lpf/led%u", index);

	return pdm_chrdev_register_lpf_device(&g_pdm_led_chrdevs[index],
					      name, nodename, device,
					      &pdm_led_fops);
}

void pdm_led_chrdev_unregister_device(const lpf_device_t *device)
{
	uint32_t index;

	if (!device || device->config.type != LPF_DEVICE_TYPE_LED)
		return;

	index = device->config.index;
	if (index >= OSAL_ARRAY_SIZE(g_pdm_led_chrdevs))
		return;

	pdm_chrdev_unregister(&g_pdm_led_chrdevs[index]);
}

void pdm_led_chrdev_record_error(uint32_t index, int error)
{
	if (index >= OSAL_ARRAY_SIZE(g_pdm_led_chrdevs))
		return;

	pdm_chrdev_record_error(&g_pdm_led_chrdevs[index], error);
}
