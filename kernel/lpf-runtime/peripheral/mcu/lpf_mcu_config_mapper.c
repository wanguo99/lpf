// SPDX-License-Identifier: GPL-2.0

#include "lpf_runtime_internal.h"

static int32_t lpf_mcu_make_device_config(
	const lpf_config_device_config_t *device, lpf_device_config_t *config)
{
	const lpf_config_mcu_entry_t *entry;
	lpf_capability_t capabilities;

	if (!device || !config)
		return OSAL_ERR_INVALID_PARAM;

	entry = (const lpf_config_mcu_entry_t *)device->entry;
	if (!entry)
		return OSAL_ERR_INVALID_PARAM;

	capabilities = LPF_DEVICE_CAP_USER_IOCTL | LPF_DEVICE_CAP_DEBUGFS;
	switch (entry->config.interface) {
	case LPF_CONFIG_MCU_INTERFACE_CAN:
		capabilities |= LPF_DEVICE_CAP_TRANSPORT_CAN;
		break;
	case LPF_CONFIG_MCU_INTERFACE_SERIAL:
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

lpf_runtime_config_mapper_register(mcu, LPF_CONFIG_DEVICE_TYPE_MCU,
				   lpf_mcu_make_device_config);
