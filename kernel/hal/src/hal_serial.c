// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "hal_serial.h"
#include "lpf/lpf_soc_adapter.h"

static void hal_serial_fill_lpf_config(const hal_serial_config_t *src,
				       lpf_serial_config_t *dst)
{
	dst->baud_rate = src->baud_rate;
	dst->data_bits = src->data_bits;
	dst->stop_bits = src->stop_bits;
	dst->parity = src->parity;
	dst->flow_control = src->flow_control;
}

int32_t hal_serial_open(const char *device, const hal_serial_config_t *config,
			hal_serial_handle_t *handle)
{
	lpf_serial_config_t lpf_config;

	if (!device || !config || !handle)
		return OSAL_ERR_INVALID_PARAM;

	hal_serial_fill_lpf_config(config, &lpf_config);
	return lpf_soc_serial_open(device, &lpf_config,
				   (lpf_serial_handle_t *)handle);
}
EXPORT_SYMBOL_GPL(hal_serial_open);

int32_t hal_serial_close(hal_serial_handle_t handle)
{
	return lpf_soc_serial_close((lpf_serial_handle_t)handle);
}
EXPORT_SYMBOL_GPL(hal_serial_close);

int32_t hal_serial_write(hal_serial_handle_t handle, const void *buffer,
			 uint32_t size, int32_t timeout)
{
	return lpf_soc_serial_write((lpf_serial_handle_t)handle, buffer, size,
				    timeout);
}
EXPORT_SYMBOL_GPL(hal_serial_write);

int32_t hal_serial_read(hal_serial_handle_t handle, void *buffer,
			uint32_t size, int32_t timeout)
{
	return lpf_soc_serial_read((lpf_serial_handle_t)handle, buffer, size,
				   timeout);
}
EXPORT_SYMBOL_GPL(hal_serial_read);

int32_t hal_serial_flush(hal_serial_handle_t handle)
{
	return lpf_soc_serial_flush((lpf_serial_handle_t)handle);
}
EXPORT_SYMBOL_GPL(hal_serial_flush);

int32_t hal_serial_set_config(hal_serial_handle_t handle,
			      const hal_serial_config_t *config)
{
	lpf_serial_config_t lpf_config;

	if (!config)
		return OSAL_ERR_INVALID_PARAM;

	hal_serial_fill_lpf_config(config, &lpf_config);
	return lpf_soc_serial_set_config((lpf_serial_handle_t)handle,
					 &lpf_config);
}
EXPORT_SYMBOL_GPL(hal_serial_set_config);
