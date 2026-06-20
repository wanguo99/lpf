// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_TRANSPORT_CAN_H
#define LPF_HW_TRANSPORT_CAN_H

#include "lpf/types/lpf_can_types.h"

int32_t lpf_hw_transport_can_init(
	const lpf_can_config_t *config,
	lpf_hw_transport_can_handle_t *handle);
int32_t lpf_hw_transport_can_deinit(lpf_hw_transport_can_handle_t handle);
int32_t lpf_hw_transport_can_send(lpf_hw_transport_can_handle_t handle,
				  const lpf_can_frame_t *frame);
int32_t lpf_hw_transport_can_recv(lpf_hw_transport_can_handle_t handle,
				  lpf_can_frame_t *frame, int32_t timeout);
int32_t lpf_hw_transport_can_set_filter(
	lpf_hw_transport_can_handle_t handle, uint32_t filter_id,
	uint32_t filter_mask);

#endif /* LPF_HW_TRANSPORT_CAN_H */
