// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_H
#define LPF_HW_H

#include "lpf/lpf_hw_types.h"
#include "lpf/lpf_hw_gpio.h"
#include "lpf/lpf_hw_pwm.h"
#include "lpf/lpf_hw_transport_can.h"
#include "lpf/lpf_hw_transport_uart.h"
#include "lpf/lpf_hw_bus_i2c.h"
#include "lpf/lpf_hw_bus_spi.h"

#define LPF_HW_VERSION_MAJOR 0x01
#define LPF_HW_VERSION_MINOR 0x00
#define LPF_HW_VERSION_PATCH 0x00

void lpf_hw_runtime_print_version(void);
int32_t lpf_hw_runtime_init(void);
void lpf_hw_runtime_exit(void);

#endif /* LPF_HW_H */
