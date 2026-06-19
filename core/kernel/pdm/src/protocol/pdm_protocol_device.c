/**
 * @file pdm_protocol_device.c
 * @brief Protocol Layer Device Messages Implementation
 * @details 统一设备协议消息实现
 */

#include "osal.h"

#include "pdm_protocol.h"

/* 设备类型名称映射表 */
static const char *device_type_names[] = {
	"UNKNOWN", /* 0x00 */
	"MCU", /* 0x01 */
};

bool pdm_protocol_device_type_valid(uint8_t dev_type)
{
	return (dev_type == PDM_PROTOCOL_DEV_TYPE_MCU);
}

const char *pdm_protocol_device_type_name(uint8_t dev_type)
{
	if (dev_type <
		OSAL_sizeof(device_type_names) / OSAL_sizeof(device_type_names[0])) {
		return device_type_names[dev_type];
	}
	return "INVALID";
}

int pdm_protocol_device_encode(pdm_protocol_encode_ctx_t *ctx)
{
	pdm_protocol_header_t *hdr;
	size_t total_len;

	/* 参数检查 */
	if (!ctx || !ctx->buffer) {
		return OSAL_ERR_INVALID_PARAM;
	}

	if (!pdm_protocol_device_type_valid(ctx->dev_type)) {
		return OSAL_ERR_INVALID_PARAM;
	}

	total_len = PDM_PROTOCOL_HEADER_SIZE + ctx->payload_len;
	if (total_len > ctx->buffer_size) {
		return OSAL_ERR_NO_MEMORY;
	}

	/* 填充协议头 */
	hdr = (pdm_protocol_header_t *)ctx->buffer;
	hdr->magic = osal_htons(PDM_PROTOCOL_MAGIC);
	hdr->version = PDM_PROTOCOL_VERSION;
	hdr->dev_type = ctx->dev_type;
	hdr->msg_type = ctx->msg_type;
	hdr->flags = ctx->flags;
	hdr->length = osal_htons(ctx->payload_len);
	hdr->seq = osal_htonl(pdm_protocol_get_next_seq());
	hdr->timestamp = osal_htonl(pdm_protocol_get_timestamp());
	hdr->reserved = 0;

	/* 复制payload */
	if (ctx->payload && ctx->payload_len > 0) {
		osal_memcpy(ctx->buffer + PDM_PROTOCOL_HEADER_SIZE, ctx->payload,
					ctx->payload_len);
	}

	pdm_protocol_set_packet_crc(ctx->buffer, total_len);

	return (int)total_len;
}

int pdm_protocol_device_decode(pdm_protocol_decode_ctx_t *ctx)
{
	const pdm_protocol_header_t *hdr;
	uint16_t magic;
	uint16_t payload_len;

	/* 参数检查 */
	if (!ctx || !ctx->buffer || ctx->buffer_len < PDM_PROTOCOL_HEADER_SIZE) {
		return OSAL_ERR_INVALID_PARAM;
	}

	hdr = (const pdm_protocol_header_t *)ctx->buffer;

	/* 验证魔数 */
	magic = osal_ntohs(hdr->magic);
	if (magic != PDM_PROTOCOL_MAGIC) {
		return OSAL_ERR_GENERIC;
	}

	/* 验证版本 */
	if (hdr->version != PDM_PROTOCOL_VERSION) {
		return OSAL_ERR_GENERIC;
	}

	/* 验证设备类型 */
	if (!pdm_protocol_device_type_valid(hdr->dev_type)) {
		return OSAL_ERR_GENERIC;
	}

	/* 获取payload长度 */
	payload_len = osal_ntohs(hdr->length);
	if (PDM_PROTOCOL_HEADER_SIZE + payload_len > ctx->buffer_len) {
		return OSAL_ERR_GENERIC;
	}

	/* 验证CRC：不修改输入缓冲区，避免破坏调用方持有的数据 */
	if (!pdm_protocol_verify_packet_crc(ctx->buffer,
					    PDM_PROTOCOL_HEADER_SIZE +
						    payload_len)) {
		return OSAL_ERR_GENERIC;
	}

	/* 输出参数 */
	ctx->dev_type = hdr->dev_type;
	ctx->msg_type = hdr->msg_type;
	ctx->flags = hdr->flags;

	ctx->payload_len = payload_len;

	/* 复制payload */
	if (payload_len > 0 && ctx->payload) {
		if (ctx->payload_size < payload_len) {
			return OSAL_ERR_NO_MEMORY;
		}
		osal_memcpy(ctx->payload, ctx->buffer + PDM_PROTOCOL_HEADER_SIZE,
			    payload_len);
	}

	return OSAL_SUCCESS;
}
