/**
 * @file pdm_protocol.c
 * @brief Protocol Layer Public API Implementation
 * @details PDM internal protocol API implementation
 */

#include "osal.h"

#include "pdm_protocol.h"

static const char *const g_pdm_protocol_device_type_names[] = {
	"UNKNOWN", /* 0x00 */
	"MCU", /* 0x01 */
	"FPGA", /* 0x02 */
};

static bool pdm_protocol_device_type_valid(uint8_t dev_type)
{
	return dev_type == PDM_PROTOCOL_DEV_TYPE_MCU ||
	       dev_type == PDM_PROTOCOL_DEV_TYPE_FPGA;
}

static const char *pdm_protocol_device_type_name(uint8_t dev_type)
{
	if (dev_type < OSAL_ARRAY_SIZE(g_pdm_protocol_device_type_names))
		return g_pdm_protocol_device_type_names[dev_type];

	return "INVALID";
}

bool pdm_protocol_is_device_type_valid(uint8_t dev_type)
{
	return pdm_protocol_device_type_valid(dev_type);
}

const char *pdm_protocol_get_device_type_name(uint8_t dev_type)
{
	return pdm_protocol_device_type_name(dev_type);
}

const char *pdm_protocol_get_error_string(int error_code)
{
	/* 现在使用 OSAL 错误码，直接返回 OSAL 错误描述 */
	return osal_get_status_name(error_code);
}

void pdm_protocol_get_version(uint8_t *major, uint8_t *minor)
{
	if (major) {
		*major = PDM_PROTOCOL_VERSION_MAJOR;
	}
	if (minor) {
		*minor = PDM_PROTOCOL_VERSION_MINOR;
	}
}

int pdm_protocol_encode(pdm_protocol_encode_ctx_t *ctx)
{
	pdm_protocol_header_t *hdr;
	size_t total_len;

	if (!ctx || !ctx->buffer)
		return OSAL_ERR_INVALID_PARAM;

	if (!pdm_protocol_device_type_valid(ctx->dev_type))
		return OSAL_ERR_INVALID_PARAM;

	total_len = PDM_PROTOCOL_HEADER_SIZE + ctx->payload_len;
	if (total_len > ctx->buffer_size)
		return OSAL_ERR_NO_MEMORY;

	hdr = (pdm_protocol_header_t *)ctx->buffer;
	pdm_protocol_init_header(hdr, ctx->dev_type, ctx->msg_type,
				 ctx->payload_len, ctx->flags);
	hdr->reserved = 0;

	if (ctx->payload && ctx->payload_len > 0)
		osal_memcpy(ctx->buffer + PDM_PROTOCOL_HEADER_SIZE,
			    ctx->payload, ctx->payload_len);

	pdm_protocol_set_packet_crc(ctx->buffer, total_len);

	return (int)total_len;
}

int pdm_protocol_decode(pdm_protocol_decode_ctx_t *ctx)
{
	const pdm_protocol_header_t *hdr;
	uint16_t payload_len;
	size_t total_len;

	if (!ctx || !ctx->buffer || ctx->buffer_len < PDM_PROTOCOL_HEADER_SIZE)
		return OSAL_ERR_INVALID_PARAM;

	hdr = (const pdm_protocol_header_t *)ctx->buffer;
	if (pdm_protocol_validate_header(hdr, 0) != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	if (!pdm_protocol_device_type_valid(hdr->dev_type))
		return OSAL_ERR_GENERIC;

	payload_len = osal_ntohs(hdr->length);
	total_len = PDM_PROTOCOL_HEADER_SIZE + payload_len;
	if (total_len > ctx->buffer_len)
		return OSAL_ERR_GENERIC;

	if (!pdm_protocol_verify_packet_crc(ctx->buffer, total_len))
		return OSAL_ERR_GENERIC;

	ctx->dev_type = hdr->dev_type;
	ctx->msg_type = hdr->msg_type;
	ctx->flags = hdr->flags;
	ctx->payload_len = payload_len;

	if (payload_len > 0 && ctx->payload) {
		if (ctx->payload_size < payload_len)
			return OSAL_ERR_NO_MEMORY;
		osal_memcpy(ctx->payload, ctx->buffer + PDM_PROTOCOL_HEADER_SIZE,
			    payload_len);
	}

	return OSAL_SUCCESS;
}
