// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_COMPAT_I2C_H
#define LPF_COMPAT_I2C_H

#include "lpf/lpf_hw_types.h"

int32_t lpf_compat_i2c_open(const char *device, lpf_i2c_handle_t *handle);
void lpf_compat_i2c_close(lpf_i2c_handle_t handle);
int32_t lpf_compat_i2c_transfer(lpf_i2c_handle_t handle,
				lpf_i2c_msg_t *msgs, uint32_t num);

#endif /* LPF_COMPAT_I2C_H */
