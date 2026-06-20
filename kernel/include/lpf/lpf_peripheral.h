// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_PERIPHERAL_H
#define LPF_PERIPHERAL_H

#include "osal.h"

int32_t lpf_peripheral_services_init(void);
void lpf_peripheral_services_exit(void);
int32_t lpf_peripheral_probe_devices(void);

#endif /* LPF_PERIPHERAL_H */
