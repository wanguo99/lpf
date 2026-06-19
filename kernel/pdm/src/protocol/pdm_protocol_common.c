/**
 * @file pdm_protocol_common.c
 * @brief Protocol Layer Common Implementation
 */

#include "osal.h"

#include "pdm_protocol.h"

/* Static storage duration leaves the atomic initialized to zero. */
static osal_atomic_uint32_t g_pdm_protocol_seq_number;

/*===========================================================================
 * 序列号和时间戳
 *===========================================================================*/

uint32_t pdm_protocol_get_next_seq(void)
{
	return osal_atomic_fetch_add(&g_pdm_protocol_seq_number, 1);
}

void pdm_protocol_reset_sequence(uint32_t seq)
{
	osal_atomic_store(&g_pdm_protocol_seq_number, seq);
}

uint32_t pdm_protocol_get_current_sequence(void)
{
	return osal_atomic_load(&g_pdm_protocol_seq_number);
}

uint32_t pdm_protocol_get_timestamp(void)
{
	OS_time_t time_struct;
	osal_get_local_time(&time_struct);
	return time_struct.seconds;
}

/*===========================================================================
 * 协议头初始化和验证
 *===========================================================================*/

void pdm_protocol_init_header(pdm_protocol_header_t *hdr, uint8_t dev_type,
			      uint8_t msg_type, uint16_t payload_len,
			      uint8_t flags)
{
	osal_memset(hdr, 0, OSAL_sizeof(pdm_protocol_header_t));

	/* 多字节字段使用网络字节序（大端），确保跨平台兼容性 */
	hdr->magic = osal_htons(PDM_PROTOCOL_MAGIC);
	hdr->version = PDM_PROTOCOL_VERSION;
	hdr->dev_type = dev_type;
	hdr->msg_type = msg_type;
	hdr->flags = flags;
	hdr->length = osal_htons(payload_len);
	hdr->seq = osal_htonl(pdm_protocol_get_next_seq());
	hdr->timestamp = osal_htonl(pdm_protocol_get_timestamp());
	/* crc16 将在 pdm_protocol_set_packet_crc() 中转换 */
}

int pdm_protocol_validate_header(const pdm_protocol_header_t *hdr,
				 uint8_t expected_dev_type)
{
	uint16_t magic;
	uint16_t length;

	/* 将网络字节序转换为主机字节序后再验证 */
	magic = osal_ntohs(hdr->magic);
	length = osal_ntohs(hdr->length);

	if (magic != PDM_PROTOCOL_MAGIC) {
		return OSAL_EPROTO; /* 协议错误：魔数不匹配 */
	}

	if ((hdr->version >> 4) != PDM_PROTOCOL_VERSION_MAJOR) {
		return OSAL_EPROTO; /* 协议错误：版本不匹配 */
	}

	if (expected_dev_type != PDM_PROTOCOL_DEV_TYPE_INVALID &&
	    hdr->dev_type != expected_dev_type) {
		return OSAL_EINVAL; /* 无效的设备类型 */
	}

	if (length > PDM_PROTOCOL_MAX_PAYLOAD_SIZE) {
		return OSAL_EINVAL; /* 无效的长度 */
	}

	return OSAL_SUCCESS;
}

/*===========================================================================
 * CRC 校验（使用 OSAL CRC 函数）
 *===========================================================================*/

uint16_t pdm_protocol_crc16(const uint8_t *data, uint16_t len)
{
	return osal_crc16_ccitt(data, len);
}

void pdm_protocol_set_packet_crc(uint8_t *packet, size_t total_len)
{
	pdm_protocol_header_t *hdr = (pdm_protocol_header_t *)packet;

	/* CRC 字段先置 0 */
	hdr->crc16 = 0;

	/* 计算整个报文的 CRC（使用 OSAL CRC16-CCITT） */
	uint16_t crc = osal_crc16_ccitt(packet, total_len);

	/* 设置 CRC（转换为网络字节序） */
	hdr->crc16 = osal_htons(crc);
}

bool pdm_protocol_verify_packet_crc(const uint8_t *packet, size_t total_len)
{
	const pdm_protocol_header_t *hdr = (const pdm_protocol_header_t *)packet;
	uint16_t received_crc = osal_ntohs(hdr->crc16); /* 转换为主机字节序 */

	/* 分段计算 CRC，避免动态内存分配
     * CRC 字段位于协议头的偏移 16-17 字节处
     * 计算顺序：[0, crc_offset) + 0x0000 + (crc_offset+2, total_len)
     */
	uint16_t crc = 0xFFFF;

	/* CRC 字段在结构体中的偏移量（手动计算以避免使用stddef.h）
     * magic(2) + version(1) + dev_type(1) + msg_type(1) + flags(1) +
     * length(2) + seq(4) + timestamp(4) = 16 字节
     */
	const size_t crc_offset = 16;

	/* 使用 OSAL 的增量 CRC 函数 */
	/* 第一段：从报文开始到 CRC 字段之前 */
	crc = osal_crc16_ccitt_update(crc, packet, crc_offset);

	/* 第二段：CRC 字段作为 0x0000 处理 */
	uint8_t zeros[2] = { 0x00, 0x00 };
	crc = osal_crc16_ccitt_update(crc, zeros, OSAL_sizeof(zeros));

	/* 第三段：从 CRC 字段之后到报文结束 */
	crc = osal_crc16_ccitt_update(crc, packet + crc_offset + 2,
				      total_len - crc_offset - 2);

	return (crc == received_crc);
}
