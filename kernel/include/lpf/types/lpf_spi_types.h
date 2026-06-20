// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_SPI_TYPES_H
#define LPF_SPI_TYPES_H

#include "osal.h"

typedef void *lpf_hw_bus_spi_handle_t;
typedef void *lpf_spi_handle_t;

#define LPF_SPI_MODE_0 0x00
#define LPF_SPI_MODE_1 0x01
#define LPF_SPI_MODE_2 0x02
#define LPF_SPI_MODE_3 0x03

typedef struct {
	const char *device;
	uint8_t mode;
	uint8_t bits_per_word;
	uint32_t max_speed_hz;
	uint32_t timeout;
} lpf_spi_config_t;

typedef struct {
	const uint8_t *tx_buf;
	uint8_t *rx_buf;
	uint32_t len;
	uint32_t speed_hz;
	uint16_t delay_usecs;
	uint8_t bits_per_word;
	uint8_t cs_change;
} lpf_spi_transfer_t;

#endif /* LPF_SPI_TYPES_H */
