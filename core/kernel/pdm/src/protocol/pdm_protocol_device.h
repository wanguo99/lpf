/**
 * @file pdm_protocol_device.h
 * @brief Protocol Layer Device Message Utilities
 */

#ifndef PDM_PROTOCOL_DEVICE_H
#define PDM_PROTOCOL_DEVICE_H

#include "pdm_protocol_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Internal Device Message Functions ========== */

/**
 * @brief 设备消息编码参数
 */
typedef struct {
	uint8_t dev_type; /**< 设备类型 */
	uint8_t msg_type; /**< 消息类型 */
	uint8_t flags; /**< 标志位 */
	const void *payload; /**< payload 数据 */
	uint16_t payload_len; /**< payload 长度 */
	uint8_t *buffer; /**< 编码后的缓冲区 */
	size_t buffer_size; /**< 缓冲区大小 */
} pdm_protocol_encode_ctx_t;

/**
 * @brief 设备消息解码参数
 */
typedef struct {
	const uint8_t *buffer; /**< 编码的数据 */
	size_t buffer_len; /**< 数据长度 */
	uint8_t dev_type; /**< 设备类型（输出） */
	uint8_t msg_type; /**< 消息类型（输出） */
	uint8_t flags; /**< 标志位（输出） */
	void *payload; /**< 解码后的 payload 缓冲区 */
	size_t payload_size; /**< payload 缓冲区大小 */
	uint32_t payload_len; /**< 实际 payload 长度（输出） */
} pdm_protocol_decode_ctx_t;

/**
 * @brief 编码设备消息
 * @param[in,out] ctx 编码参数
 * @return 成功返回编码长度（>0），失败返回 OSAL 错误码
 */
int pdm_protocol_device_encode(pdm_protocol_encode_ctx_t *ctx);

/**
 * @brief 解码设备消息
 * @param[in,out] ctx 解码参数
 * @return OSAL_SUCCESS 成功，OSAL_ERR_* 失败
 */
int pdm_protocol_device_decode(pdm_protocol_decode_ctx_t *ctx);

bool pdm_protocol_device_type_valid(uint8_t dev_type);
const char *pdm_protocol_device_type_name(uint8_t dev_type);

#ifdef __cplusplus
}
#endif

#endif /* PDM_PROTOCOL_DEVICE_H */
