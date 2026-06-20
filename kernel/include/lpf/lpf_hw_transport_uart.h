// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_TRANSPORT_UART_H
#define LPF_HW_TRANSPORT_UART_H

#include "lpf/lpf_hw_types.h"

int32_t lpf_hw_transport_uart_open(
	const char *device, const lpf_serial_config_t *config,
	lpf_hw_transport_uart_handle_t *handle);
int32_t lpf_hw_transport_uart_close(lpf_hw_transport_uart_handle_t handle);
int32_t lpf_hw_transport_uart_write(lpf_hw_transport_uart_handle_t handle,
				    const void *buffer, uint32_t size,
				    int32_t timeout);
int32_t lpf_hw_transport_uart_read(lpf_hw_transport_uart_handle_t handle,
				   void *buffer, uint32_t size,
				   int32_t timeout);
int32_t lpf_hw_transport_uart_flush(lpf_hw_transport_uart_handle_t handle);
int32_t lpf_hw_transport_uart_set_config(
	lpf_hw_transport_uart_handle_t handle,
	const lpf_serial_config_t *config);

#endif /* LPF_HW_TRANSPORT_UART_H */
