// SPDX-License-Identifier: GPL-2.0

#include "lpf/lpf_mcu_transport.h"

#ifdef CONFIG_LPF_MCU_TRANSPORT_CAN
extern const lpf_mcu_transport_ops_t lpf_mcu_transport_can_ops;
#endif

#ifdef CONFIG_LPF_MCU_TRANSPORT_UART
extern const lpf_mcu_transport_ops_t lpf_mcu_transport_uart_ops;
#endif

const lpf_mcu_transport_ops_t *
lpf_mcu_transport_get(pconfig_mcu_interface_t interface)
{
	switch (interface) {
	case PCONFIG_MCU_INTERFACE_CAN:
#ifdef CONFIG_LPF_MCU_TRANSPORT_CAN
		return &lpf_mcu_transport_can_ops;
#else
		return NULL;
#endif
	case PCONFIG_MCU_INTERFACE_SERIAL:
#ifdef CONFIG_LPF_MCU_TRANSPORT_UART
		return &lpf_mcu_transport_uart_ops;
#else
		return NULL;
#endif
	default:
		return NULL;
	}
}
