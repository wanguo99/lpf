// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_I2C_H
#define LPF_HW_I2C_H

#include "lpf/types/lpf_i2c_types.h"

int32_t lpf_hw_bus_i2c_open(const lpf_i2c_config_t *config,
			    lpf_hw_bus_i2c_handle_t *handle);
int32_t lpf_hw_bus_i2c_close(lpf_hw_bus_i2c_handle_t handle);
int32_t lpf_hw_bus_i2c_write(lpf_hw_bus_i2c_handle_t handle,
			     uint16_t slave_addr, const uint8_t *buffer,
			     uint32_t size);
int32_t lpf_hw_bus_i2c_read(lpf_hw_bus_i2c_handle_t handle,
			    uint16_t slave_addr, uint8_t *buffer,
			    uint32_t size);
int32_t lpf_hw_bus_i2c_write_reg(lpf_hw_bus_i2c_handle_t handle,
				 uint16_t slave_addr, uint8_t reg_addr,
				 const uint8_t *buffer, uint32_t size);
int32_t lpf_hw_bus_i2c_read_reg(lpf_hw_bus_i2c_handle_t handle,
				uint16_t slave_addr, uint8_t reg_addr,
				uint8_t *buffer, uint32_t size);
int32_t lpf_hw_bus_i2c_transfer(lpf_hw_bus_i2c_handle_t handle,
				lpf_i2c_msg_t *msgs, uint32_t num);

#endif /* LPF_HW_I2C_H */
