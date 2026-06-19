/**
 * @file pdm_protocol.h
 * @brief PDM internal protocol entry point
 */

#ifndef PDM_PROTOCOL_H
#define PDM_PROTOCOL_H

/* Core protocol definitions - always required */
#include "pdm_protocol_common.h"
#include "pdm_protocol_device.h"

/* Device-specific protocol - current MCU path */
#ifdef CONFIG_PDM_PROTOCOL_MCU
#include "pdm_protocol_mcu.h"
#endif /* CONFIG_PDM_PROTOCOL_MCU */

int pdm_protocol_init(void);
int pdm_protocol_deinit(void);

#endif /* PDM_PROTOCOL_H */
