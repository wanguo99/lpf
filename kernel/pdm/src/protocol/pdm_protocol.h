/**
 * @file pdm_protocol.h
 * @brief PDM standard peripheral protocol entry point
 */

#ifndef PDM_PROTOCOL_H
#define PDM_PROTOCOL_H

/* Core protocol definitions - always required */
#include "pdm_protocol_common.h"

/* Device-specific protocol - current MCU path */
#include "pdm_protocol_mcu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t dev_type;
	uint8_t msg_type;
	uint8_t flags;
	const void *payload;
	uint16_t payload_len;
	uint8_t *buffer;
	size_t buffer_size;
} pdm_protocol_encode_ctx_t;

typedef struct {
	const uint8_t *buffer;
	size_t buffer_len;
	uint8_t dev_type;
	uint8_t msg_type;
	uint8_t flags;
	void *payload;
	size_t payload_size;
	uint32_t payload_len;
} pdm_protocol_decode_ctx_t;

int pdm_protocol_encode(pdm_protocol_encode_ctx_t *ctx);
int pdm_protocol_decode(pdm_protocol_decode_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* PDM_PROTOCOL_H */
