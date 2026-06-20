// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_COMPAT_SERIAL_H
#define LPF_COMPAT_SERIAL_H

#include "lpf/types/lpf_serial_types.h"

int32_t lpf_compat_serial_open(const char *device,
			       const lpf_serial_config_t *config,
			       lpf_serial_handle_t *handle);
int32_t lpf_compat_serial_close(lpf_serial_handle_t handle);
int32_t lpf_compat_serial_write(lpf_serial_handle_t handle,
				const void *buffer, uint32_t size,
				int32_t timeout);
int32_t lpf_compat_serial_read(lpf_serial_handle_t handle, void *buffer,
			       uint32_t size, int32_t timeout);
int32_t lpf_compat_serial_flush(lpf_serial_handle_t handle);
int32_t lpf_compat_serial_set_config(lpf_serial_handle_t handle,
				     const lpf_serial_config_t *config);

#endif /* LPF_COMPAT_SERIAL_H */
