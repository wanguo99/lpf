/**
 * @file lpf_protocol.h
 * @brief LPF standard peripheral protocol entry point
 */

#ifndef LPF_PROTOCOL_H
#define LPF_PROTOCOL_H

#include "osal.h"

#define LPF_PROTOCOL_MAGIC 0xAA55
#define LPF_PROTOCOL_VERSION 0x01
#define LPF_PROTOCOL_VERSION_MAJOR 0x00
#define LPF_PROTOCOL_VERSION_MINOR 0x01
#define LPF_PROTOCOL_HEADER_SIZE 20

#ifdef CONFIG_LPF_PROTOCOL_MAX_PAYLOAD_SIZE
#define LPF_PROTOCOL_MAX_PAYLOAD_SIZE CONFIG_LPF_PROTOCOL_MAX_PAYLOAD_SIZE
#else
#define LPF_PROTOCOL_MAX_PAYLOAD_SIZE 1024
#endif

#define LPF_PROTOCOL_MAX_PACKET_SIZE \
	(LPF_PROTOCOL_HEADER_SIZE + LPF_PROTOCOL_MAX_PAYLOAD_SIZE)

typedef enum {
	LPF_PROTOCOL_DEV_TYPE_INVALID = 0x00,
	LPF_PROTOCOL_DEV_TYPE_MCU = 0x01,
	LPF_PROTOCOL_DEV_TYPE_FPGA = 0x02,
} lpf_protocol_dev_type_t;

typedef struct {
	uint16_t magic;
	uint8_t version;
	uint8_t dev_type;
	uint8_t msg_type;
	uint8_t flags;
	uint16_t length;
	uint32_t seq;
	uint32_t timestamp;
	uint16_t crc16;
	uint16_t reserved;
} __attribute__((packed)) lpf_protocol_header_t;

#include "lpf/lpf_protocol_mcu.h"

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
} lpf_protocol_encode_ctx_t;

typedef struct {
	const uint8_t *buffer;
	size_t buffer_len;
	uint8_t dev_type;
	uint8_t msg_type;
	uint8_t flags;
	void *payload;
	size_t payload_size;
	uint32_t payload_len;
} lpf_protocol_decode_ctx_t;

int lpf_protocol_encode(lpf_protocol_encode_ctx_t *ctx);
int lpf_protocol_decode(lpf_protocol_decode_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* LPF_PROTOCOL_H */
