// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_SPI_H
#define LPF_HW_SPI_H

#include "lpf/types/lpf_spi_types.h"

int32_t lpf_hw_bus_spi_open(const lpf_spi_config_t *config,
			    lpf_hw_bus_spi_handle_t *handle);
int32_t lpf_hw_bus_spi_close(lpf_hw_bus_spi_handle_t handle);
int32_t lpf_hw_bus_spi_write(lpf_hw_bus_spi_handle_t handle,
			     const uint8_t *buffer, uint32_t size);
int32_t lpf_hw_bus_spi_read(lpf_hw_bus_spi_handle_t handle,
			    uint8_t *buffer, uint32_t size);
int32_t lpf_hw_bus_spi_transfer(lpf_hw_bus_spi_handle_t handle,
				const uint8_t *tx_buffer, uint8_t *rx_buffer,
				uint32_t size);
int32_t lpf_hw_bus_spi_transfer_multi(lpf_hw_bus_spi_handle_t handle,
				      lpf_spi_transfer_t *transfers,
				      uint32_t num);
int32_t lpf_hw_bus_spi_set_config(lpf_hw_bus_spi_handle_t handle,
				  const lpf_spi_config_t *config);

#endif /* LPF_HW_SPI_H */
