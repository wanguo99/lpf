// SPDX-License-Identifier: GPL-2.0

#include "lpf/runtime/lpf_runtime.h"

#include "lpf/core/lpf_core.h"
#include "lpf_runtime_internal.h"

static const lpf_runtime_config_mapper_t *
lpf_runtime_config_mapper_first(void)
{
	return &lpf_runtime_config_mapper_start + 1;
}

static const lpf_runtime_config_mapper_t *
lpf_runtime_config_mapper_last(void)
{
	return &lpf_runtime_config_mapper_end;
}

static int32_t lpf_peripheral_make_device_config(
	const lpf_config_device_config_t *device, lpf_device_config_t *config)
{
	const lpf_runtime_config_mapper_t *mapper;

	if (!device || !config)
		return OSAL_ERR_INVALID_PARAM;

	osal_memset(config, 0, sizeof(*config));

	for (mapper = lpf_runtime_config_mapper_first();
	     mapper < lpf_runtime_config_mapper_last(); mapper++) {
		if (mapper->type != device->device_type)
			continue;
		if (!mapper->map)
			return OSAL_ERR_INVALID_PARAM;
		return mapper->map(device, config);
	}

	return OSAL_ERR_NOT_SUPPORTED;
}

int32_t lpf_runtime_probe_devices(void)
{
	const lpf_config_device_config_t *device;
	int32_t ret;

	ret = lpf_config_load();
	if (ret != OSAL_SUCCESS)
		return ret;

	device = lpf_config_get();
	if (!device)
		return OSAL_ENODEV;

	for (; device->device_type != LPF_CONFIG_DEVICE_TYPE_INVALID; device++) {
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
