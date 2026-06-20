// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_PERIPHERAL_H
#define LPF_PERIPHERAL_H

#include "osal.h"

#define LPF_PERIPHERAL_RUNTIME_VERSION_MAJOR 0x01
#define LPF_PERIPHERAL_RUNTIME_VERSION_MINOR 0x00
#define LPF_PERIPHERAL_RUNTIME_VERSION_PATCH 0x00

void lpf_peripheral_runtime_print_version(void);
int32_t lpf_peripheral_runtime_init(void);
void lpf_peripheral_runtime_exit(void);

#endif /* LPF_PERIPHERAL_H */
