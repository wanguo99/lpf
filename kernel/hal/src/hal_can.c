// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "hal_can.h"
#include "lpf/lpf_soc_adapter.h"

static void hal_can_fill_lpf_config(const hal_can_config_t *src,
				    lpf_can_config_t *dst)
{
	dst->interface = src->interface;
	dst->baudrate = src->baudrate;
	dst->rx_timeout = src->rx_timeout;
	dst->tx_timeout = src->tx_timeout;
}

static void hal_can_fill_lpf_frame(const hal_can_frame_t *src,
				   lpf_can_frame_t *dst)
{
	dst->can_id = src->can_id;
	dst->dlc = src->dlc;
	osal_memcpy(dst->data, src->data, sizeof(dst->data));
	dst->timestamp = src->timestamp;
}

static void hal_can_fill_frame(const lpf_can_frame_t *src,
			       hal_can_frame_t *dst)
{
	dst->can_id = src->can_id;
	dst->dlc = src->dlc;
	osal_memcpy(dst->data, src->data, sizeof(dst->data));
	dst->timestamp = src->timestamp;
}

int32_t hal_can_init(const hal_can_config_t *config, hal_can_handle_t *handle)
{
	lpf_can_config_t lpf_config;

	if (!config || !handle)
		return OSAL_ERR_INVALID_PARAM;

	hal_can_fill_lpf_config(config, &lpf_config);
	return lpf_soc_can_init(&lpf_config, (lpf_can_handle_t *)handle);
}
EXPORT_SYMBOL_GPL(hal_can_init);

int32_t hal_can_deinit(hal_can_handle_t handle)
{
	return lpf_soc_can_deinit((lpf_can_handle_t)handle);
}
EXPORT_SYMBOL_GPL(hal_can_deinit);

int32_t hal_can_send(hal_can_handle_t handle, const hal_can_frame_t *frame)
{
	lpf_can_frame_t lpf_frame;

	if (!handle || !frame)
		return OSAL_ERR_INVALID_PARAM;

	if (frame->dlc > HAL_CAN_MAX_DATA_LEN)
		return OSAL_ERR_INVALID_SIZE;

	hal_can_fill_lpf_frame(frame, &lpf_frame);
	return lpf_soc_can_send((lpf_can_handle_t)handle, &lpf_frame);
}
EXPORT_SYMBOL_GPL(hal_can_send);

int32_t hal_can_recv(hal_can_handle_t handle, hal_can_frame_t *frame,
		     int32_t timeout)
{
	lpf_can_frame_t lpf_frame;
	int32_t ret;

	if (!handle || !frame)
		return OSAL_ERR_INVALID_PARAM;

	ret = lpf_soc_can_recv((lpf_can_handle_t)handle, &lpf_frame, timeout);
	if (ret != OSAL_SUCCESS)
		return ret;

	hal_can_fill_frame(&lpf_frame, frame);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_can_recv);

int32_t hal_can_set_filter(hal_can_handle_t handle, uint32_t filter_id,
			   uint32_t filter_mask)
{
	return lpf_soc_can_set_filter((lpf_can_handle_t)handle, filter_id,
				      filter_mask);
}
EXPORT_SYMBOL_GPL(hal_can_set_filter);
