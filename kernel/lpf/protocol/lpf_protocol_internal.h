// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_PROTOCOL_INTERNAL_H
#define LPF_PROTOCOL_INTERNAL_H

#include "lpf/lpf_protocol.h"

uint16_t lpf_protocol_crc16(const uint8_t *data, uint16_t len);
uint32_t lpf_protocol_get_next_seq(void);
uint32_t lpf_protocol_get_timestamp(void);
void lpf_protocol_reset_sequence(uint32_t seq);
uint32_t lpf_protocol_get_current_sequence(void);
void lpf_protocol_init_header(lpf_protocol_header_t *hdr, uint8_t dev_type,
			      uint8_t msg_type, uint16_t payload_len,
			      uint8_t flags);
int lpf_protocol_validate_header(const lpf_protocol_header_t *hdr,
				 uint8_t expected_dev_type);
void lpf_protocol_set_packet_crc(uint8_t *packet, size_t total_len);
bool lpf_protocol_verify_packet_crc(const uint8_t *packet, size_t total_len);
bool lpf_protocol_is_device_type_valid(uint8_t dev_type);
const char *lpf_protocol_get_device_type_name(uint8_t dev_type);
const char *lpf_protocol_get_error_string(int32_t error_code);
void lpf_protocol_get_version(uint8_t *major, uint8_t *minor);

#endif /* LPF_PROTOCOL_INTERNAL_H */
