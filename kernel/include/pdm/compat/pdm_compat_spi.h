// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_COMPAT_SPI_H
#define LPF_COMPAT_SPI_H

#include "lpf/types/lpf_spi_types.h"

int32_t lpf_compat_spi_open(const lpf_spi_config_t *config,
			    lpf_spi_handle_t *handle);
void lpf_compat_spi_close(lpf_spi_handle_t handle);
int32_t lpf_compat_spi_transfer(lpf_spi_handle_t handle,
				const lpf_spi_transfer_t *transfers,
				uint32_t num);
int32_t lpf_compat_spi_set_config(lpf_spi_handle_t handle,
				  const lpf_spi_config_t *config);

#endif /* LPF_COMPAT_SPI_H */
