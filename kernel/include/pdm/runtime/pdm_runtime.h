// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_RUNTIME_H
#define LPF_RUNTIME_H

#include "osal.h"

#define LPF_RUNTIME_VERSION_MAJOR 0x01
#define LPF_RUNTIME_VERSION_MINOR 0x00
#define LPF_RUNTIME_VERSION_PATCH 0x00

void lpf_runtime_print_version(void);
int32_t lpf_runtime_init(void);
void lpf_runtime_exit(void);
int32_t lpf_runtime_config_refresh(void);
void lpf_runtime_config_detach(void);

#endif /* LPF_RUNTIME_H */
