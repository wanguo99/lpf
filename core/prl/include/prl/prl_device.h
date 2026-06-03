/**
#include "osal_types.h"
 * @file prl_device.h
 * @brief Protocol Layer Device Message Utilities
 */

#ifndef PRL_PRL_DEVICE_H
#define PRL_PRL_DEVICE_H

#include "prl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Internal Device Message Functions ========== */

int prl_device_encode(uint8_t dev_type, uint8_t msg_type,
                      const void *payload, uint16_t payload_len,
                      uint8_t *buffer, size_t buffer_size, uint8_t flags);

int prl_device_decode(const uint8_t *packet, size_t packet_len,
                      uint8_t *dev_type, uint8_t *msg_type,
                      const uint8_t **payload, uint16_t *payload_len);

bool prl_device_type_valid(uint8_t dev_type);
const char *prl_device_type_name(uint8_t dev_type);

#ifdef __cplusplus
}
#endif

#endif /* PRL_PRL_DEVICE_H */
