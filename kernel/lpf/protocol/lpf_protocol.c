/**
 * @file lpf_protocol.c
 * @brief Protocol Layer Public API Implementation
 * @details LPF protocol API implementation
 */

#include "osal.h"

#include "lpf_protocol_internal.h"

static const char *const g_lpf_protocol_device_type_names[] = {
	"UNKNOWN", /* 0x00 */
	"MCU", /* 0x01 */
	"FPGA", /* 0x02 */
};

static bool lpf_protocol_device_type_valid(uint8_t dev_type)
{
	return dev_type == LPF_PROTOCOL_DEV_TYPE_MCU ||
	       dev_type == LPF_PROTOCOL_DEV_TYPE_FPGA;
}

static const char *lpf_protocol_device_type_name(uint8_t dev_type)
{
	if (dev_type < OSAL_ARRAY_SIZE(g_lpf_protocol_device_type_names))
		return g_lpf_protocol_device_type_names[dev_type];

	return "INVALID";
}

bool lpf_protocol_is_device_type_valid(uint8_t dev_type)
{
	return lpf_protocol_device_type_valid(dev_type);
}

const char *lpf_protocol_get_device_type_name(uint8_t dev_type)
{
	return lpf_protocol_device_type_name(dev_type);
}

const char *lpf_protocol_get_error_string(int error_code)
{
	/* 现在使用 OSAL 错误码，直接返回 OSAL 错误描述 */
	return osal_get_status_name(error_code);
}

void lpf_protocol_get_version(uint8_t *major, uint8_t *minor)
{
	if (major) {
		*major = LPF_PROTOCOL_VERSION_MAJOR;
	}
	if (minor) {
		*minor = LPF_PROTOCOL_VERSION_MINOR;
	}
}

int lpf_protocol_encode(lpf_protocol_encode_ctx_t *ctx)
{
	lpf_protocol_header_t *hdr;
	size_t total_len;

	if (!ctx || !ctx->buffer)
		return OSAL_ERR_INVALID_PARAM;

	if (!lpf_protocol_device_type_valid(ctx->dev_type))
		return OSAL_ERR_INVALID_PARAM;

	total_len = LPF_PROTOCOL_HEADER_SIZE + ctx->payload_len;
	if (total_len > ctx->buffer_size)
		return OSAL_ERR_NO_MEMORY;

	hdr = (lpf_protocol_header_t *)ctx->buffer;
	lpf_protocol_init_header(hdr, ctx->dev_type, ctx->msg_type,
				 ctx->payload_len, ctx->flags);
	hdr->reserved = 0;

	if (ctx->payload && ctx->payload_len > 0)
		osal_memcpy(ctx->buffer + LPF_PROTOCOL_HEADER_SIZE,
			    ctx->payload, ctx->payload_len);

	lpf_protocol_set_packet_crc(ctx->buffer, total_len);

	return (int)total_len;
}
EXPORT_SYMBOL_GPL(lpf_protocol_encode);

int lpf_protocol_decode(lpf_protocol_decode_ctx_t *ctx)
{
	const lpf_protocol_header_t *hdr;
	uint16_t payload_len;
	size_t total_len;

	if (!ctx || !ctx->buffer || ctx->buffer_len < LPF_PROTOCOL_HEADER_SIZE)
		return OSAL_ERR_INVALID_PARAM;

	hdr = (const lpf_protocol_header_t *)ctx->buffer;
	if (lpf_protocol_validate_header(hdr, 0) != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	if (!lpf_protocol_device_type_valid(hdr->dev_type))
		return OSAL_ERR_GENERIC;

	payload_len = osal_ntohs(hdr->length);
	total_len = LPF_PROTOCOL_HEADER_SIZE + payload_len;
	if (total_len > ctx->buffer_len)
		return OSAL_ERR_GENERIC;

	if (!lpf_protocol_verify_packet_crc(ctx->buffer, total_len))
		return OSAL_ERR_GENERIC;

	ctx->dev_type = hdr->dev_type;
	ctx->msg_type = hdr->msg_type;
	ctx->flags = hdr->flags;
	ctx->payload_len = payload_len;

	if (payload_len > 0 && ctx->payload) {
		if (ctx->payload_size < payload_len)
			return OSAL_ERR_NO_MEMORY;
		osal_memcpy(ctx->payload, ctx->buffer + LPF_PROTOCOL_HEADER_SIZE,
			    payload_len);
	}

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_protocol_decode);
