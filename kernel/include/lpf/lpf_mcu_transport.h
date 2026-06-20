// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_MCU_TRANSPORT_H
#define LPF_MCU_TRANSPORT_H

#include "osal.h"
#include "pconfig.h"

typedef void *lpf_mcu_transport_handle_t;

typedef struct {
	pconfig_mcu_interface_t interface;
	const char *name;
	int32_t (*open)(const pconfig_mcu_config_t *config,
			lpf_mcu_transport_handle_t *handle);
	int32_t (*close)(lpf_mcu_transport_handle_t handle);
	int32_t (*transfer)(lpf_mcu_transport_handle_t handle,
			    const uint8_t *packet, uint32_t packet_len,
			    uint8_t *response, uint32_t response_size,
			    uint32_t *actual_size, uint32_t timeout_ms);
} lpf_mcu_transport_ops_t;

const lpf_mcu_transport_ops_t *
lpf_mcu_transport_get(pconfig_mcu_interface_t interface);

#endif /* LPF_MCU_TRANSPORT_H */
