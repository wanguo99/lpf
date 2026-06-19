// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/errno.h>

#include "osal.h"
#include "pdm/pdm.h"
#include "pconfig/pconfig.h"
#include "pdm_driver.h"
#include "pdm_ctl.h"
#include "pdm_status.h"
#include "generated/gen_version.h"

static int32_t pdm_make_lpf_mcu_config(const pconfig_device_config_t *device,
				       lpf_device_config_t *config)
{
	const pconfig_mcu_entry_t *entry;
	lpf_capability_t capabilities;

	entry = (const pconfig_mcu_entry_t *)device->entry;
	if (!entry)
		return OSAL_ERR_INVALID_PARAM;

	capabilities = LPF_DEVICE_CAP_USER_IOCTL | LPF_DEVICE_CAP_DEBUG_PROCFS;
	switch (entry->config.interface) {
	case PCONFIG_MCU_INTERFACE_CAN:
		capabilities |= LPF_DEVICE_CAP_TRANSPORT_CAN;
		break;
	case PCONFIG_MCU_INTERFACE_SERIAL:
		capabilities |= LPF_DEVICE_CAP_TRANSPORT_UART;
		break;
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}

	config->type = LPF_DEVICE_TYPE_MCU;
	config->index = device->index;
	config->entry = entry;
	config->name = entry->config.name;
	config->capabilities = capabilities;
	return OSAL_SUCCESS;
}

static int32_t pdm_make_lpf_led_config(const pconfig_device_config_t *device,
				       lpf_device_config_t *config)
{
	const pconfig_led_entry_t *entry;
	lpf_capability_t capabilities;

	entry = (const pconfig_led_entry_t *)device->entry;
	if (!entry)
		return OSAL_ERR_INVALID_PARAM;

	capabilities = LPF_DEVICE_CAP_USER_IOCTL | LPF_DEVICE_CAP_DEBUG_PROCFS;
	switch (entry->config.control) {
	case PCONFIG_LED_CONTROL_GPIO:
		capabilities |= LPF_DEVICE_CAP_CONTROL_GPIO;
		break;
	case PCONFIG_LED_CONTROL_PWM:
		capabilities |= LPF_DEVICE_CAP_CONTROL_PWM;
		break;
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}

	config->type = LPF_DEVICE_TYPE_LED;
	config->index = device->index;
	config->entry = entry;
	config->name = entry->config.name;
	config->capabilities = capabilities;
	return OSAL_SUCCESS;
}

static int32_t pdm_make_lpf_device_config(
	const pconfig_device_config_t *device, lpf_device_config_t *config)
{
	if (!device || !config)
		return OSAL_ERR_INVALID_PARAM;

	osal_memset(config, 0, sizeof(*config));

	switch (device->device_type) {
	case PCONFIG_DEVICE_TYPE_MCU:
		return pdm_make_lpf_mcu_config(device, config);
	case PCONFIG_DEVICE_TYPE_LED:
		return pdm_make_lpf_led_config(device, config);
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}
}

static int pdm_probe_devices(void)
{
	const pconfig_device_config_t *device;
	int32_t ret;

	ret = pconfig_load();
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	device = pconfig_get();
	if (!device)
		return -ENODEV;

	for (; device->device_type != PCONFIG_DEVICE_TYPE_INVALID; device++) {
		lpf_device_config_t config;

		ret = pdm_make_lpf_device_config(device, &config);
		if (ret != OSAL_SUCCESS) {
			ret = pdm_status_to_errno(ret);
			goto out_error;
		}

		ret = lpf_device_register(&config);
		if (ret != OSAL_SUCCESS) {
			ret = pdm_status_to_errno(ret);
			goto out_error;
		}
	}

	return 0;

out_error:
	lpf_device_unregister_all();
	return ret;
}

void pdm_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "PDM",
		 "module_version=%u.%u.%u lpf_version=%s git=%s build_time=%s build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 PDM_VERSION_MAJOR, PDM_VERSION_MINOR, PDM_VERSION_PATCH,
		 LPF_VERSION, LPF_GIT_COMMIT,
		 LPF_COMPILE_TIME, LPF_COMPILE_BY,
		 LPF_COMPILE_HOST, LPF_COMPILER,
		 LPF_BUILD_ARCH, LPF_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(pdm_print_version);

static int __init pdm_init(void)
{
	int ret;

	pdm_print_version();

	ret = pdm_driver_registry_init();
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	ret = pdm_drivers_init();
	if (ret)
		goto out_registry_deinit;

	ret = pdm_probe_devices();
	if (ret) {
		pdm_drivers_exit();
		goto out_registry_deinit;
	}

	ret = pdm_ctl_chrdev_register();
	if (ret) {
		lpf_device_unregister_all();
		pdm_drivers_exit();
		goto out_registry_deinit;
	}

	LOG_INFO("PDM", "loaded");
	return 0;

out_registry_deinit:
	pdm_driver_registry_deinit();
	return ret;
}

static void __exit pdm_exit(void)
{
	pdm_ctl_chrdev_unregister();
	lpf_device_unregister_all();
	pdm_drivers_exit();
	pdm_driver_registry_deinit();
	LOG_INFO("PDM", "unloaded");
}

module_init(pdm_init);
module_exit(pdm_exit);

MODULE_AUTHOR("LPF");
MODULE_DESCRIPTION("LPF PDM kernel module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal lpf_core pconfig hal can can_raw");
