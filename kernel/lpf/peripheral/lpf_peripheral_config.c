// SPDX-License-Identifier: GPL-2.0

#include "lpf/lpf_peripheral.h"

#include "lpf/lpf_core.h"
#include "pconfig/pconfig.h"

static int32_t lpf_peripheral_make_mcu_config(
	const pconfig_device_config_t *device, lpf_device_config_t *config)
{
	const pconfig_mcu_entry_t *entry;
	lpf_capability_t capabilities;

	entry = (const pconfig_mcu_entry_t *)device->entry;
	if (!entry)
		return OSAL_ERR_INVALID_PARAM;

	capabilities = LPF_DEVICE_CAP_USER_IOCTL | LPF_DEVICE_CAP_DEBUGFS;
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

static int32_t lpf_peripheral_make_led_config(
	const pconfig_device_config_t *device, lpf_device_config_t *config)
{
	const pconfig_led_entry_t *entry;
	lpf_capability_t capabilities;

	entry = (const pconfig_led_entry_t *)device->entry;
	if (!entry)
		return OSAL_ERR_INVALID_PARAM;

	capabilities = LPF_DEVICE_CAP_USER_IOCTL | LPF_DEVICE_CAP_DEBUGFS;
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

static int32_t lpf_peripheral_make_device_config(
	const pconfig_device_config_t *device, lpf_device_config_t *config)
{
	if (!device || !config)
		return OSAL_ERR_INVALID_PARAM;

	osal_memset(config, 0, sizeof(*config));

	switch (device->device_type) {
	case PCONFIG_DEVICE_TYPE_MCU:
		return lpf_peripheral_make_mcu_config(device, config);
	case PCONFIG_DEVICE_TYPE_LED:
		return lpf_peripheral_make_led_config(device, config);
	default:
		return OSAL_ERR_NOT_SUPPORTED;
	}
}

int32_t lpf_peripheral_probe_devices(void)
{
	const pconfig_device_config_t *device;
	int32_t ret;

	ret = pconfig_load();
	if (ret != OSAL_SUCCESS)
		return ret;

	device = pconfig_get();
	if (!device)
		return OSAL_ENODEV;

	for (; device->device_type != PCONFIG_DEVICE_TYPE_INVALID; device++) {
		lpf_device_config_t config;

		ret = lpf_peripheral_make_device_config(device, &config);
		if (ret != OSAL_SUCCESS)
			goto out_error;

		ret = lpf_device_register(&config);
		if (ret != OSAL_SUCCESS)
			goto out_error;
	}

	return OSAL_SUCCESS;

out_error:
	lpf_device_unregister_all();
	return ret;
}
