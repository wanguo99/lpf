// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_H
#define LPF_HW_H

#include "lpf/hw/lpf_hw_gpio.h"
#include "lpf/hw/lpf_hw_pwm.h"
#include "lpf/hw/lpf_hw_can.h"
#include "lpf/hw/lpf_hw_uart.h"
#include "lpf/hw/lpf_hw_i2c.h"
#include "lpf/hw/lpf_hw_spi.h"

#define LPF_HW_VERSION_MAJOR 0x01
#define LPF_HW_VERSION_MINOR 0x00
#define LPF_HW_VERSION_PATCH 0x00

int32_t lpf_hw_runtime_init(void);
void lpf_hw_runtime_exit(void);

#endif /* LPF_HW_H */
