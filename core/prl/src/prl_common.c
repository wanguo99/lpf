/**
 * @file prl_common.c
 * @brief Protocol Layer Common Implementation
 */

#include "osal.h"

#include "prl.h"


/* 全局序列号（非静态，供 prl_api.c 访问） */
osal_atomic_uint32_t g_seq_number;

/* 初始化序列号（需要在模块加载时调用） */
__attribute__((constructor)) static void prl_init_seq(void)
{
	OSAL_atomic_init(&g_seq_number, 0);
}

/*===========================================================================
 * 序列号和时间戳
 *===========================================================================*/

uint32_t prl_get_next_seq(void)
{
    return OSAL_atomic_fetch_add(&g_seq_number, 1);
}

uint32_t prl_get_timestamp(void)
{
    OS_time_t time_struct;
    OSAL_get_local_time(&time_struct);
    return time_struct.seconds;
}

/*===========================================================================
 * 协议头初始化和验证
 *===========================================================================*/

void prl_init_header(prl_header_t *hdr, uint8_t dev_type, uint8_t msg_type,
                     uint16_t payload_len, uint8_t flags)
{
    OSAL_memset(hdr, 0, OSAL_sizeof(prl_header_t));

    /* 多字节字段使用网络字节序（大端），确保跨平台兼容性 */
    hdr->magic = OSAL_htons(PRL_MAGIC);
    hdr->version = PRL_VERSION;
    hdr->dev_type = dev_type;
    hdr->msg_type = msg_type;
    hdr->flags = flags;
    hdr->length = OSAL_htons(payload_len);
    hdr->seq = OSAL_htonl(prl_get_next_seq());
    hdr->timestamp = OSAL_htonl(prl_get_timestamp());
    /* crc16 将在 prl_set_packet_crc() 中转换 */
}

int prl_validate_header(const prl_header_t *hdr, uint8_t expected_type)
{
    uint16_t magic;
    uint16_t length;

    /* 将网络字节序转换为主机字节序后再验证 */
    magic = OSAL_ntohs(hdr->magic);
    length = OSAL_ntohs(hdr->length);

    if (magic != PRL_MAGIC) {
        return OSAL_EPROTO;  /* 协议错误：魔数不匹配 */
    }

    if ((hdr->version >> 4) != PRL_VERSION_MAJOR) {
        return OSAL_EPROTO;  /* 协议错误：版本不匹配 */
    }

    if (expected_type != 0 && hdr->msg_type != expected_type) {
        return OSAL_EINVAL;  /* 无效的设备类型 */
    }

    if (length > PRL_MAX_PAYLOAD_SIZE) {
        return OSAL_EINVAL;  /* 无效的长度 */
    }

    return OSAL_SUCCESS;
}

/*===========================================================================
 * CRC 校验（使用 OSAL CRC 函数）
 *===========================================================================*/

void prl_set_packet_crc(uint8_t *packet, size_t total_len)
{
    prl_header_t *hdr = (prl_header_t *)packet;

    /* CRC 字段先置 0 */
    hdr->crc16 = 0;

    /* 计算整个报文的 CRC（使用 OSAL CRC16-CCITT） */
    uint16_t crc = OSAL_CRC16_CCITT(packet, total_len);

    /* 设置 CRC（转换为网络字节序） */
    hdr->crc16 = OSAL_htons(crc);
}

bool prl_verify_packet_crc(const uint8_t *packet, size_t total_len)
{
    const prl_header_t *hdr = (const prl_header_t *)packet;
    uint16_t received_crc = OSAL_ntohs(hdr->crc16);  /* 转换为主机字节序 */

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
    crc = OSAL_CRC16_CCITT_Update(crc, packet, crc_offset);

    /* 第二段：CRC 字段作为 0x0000 处理 */
    uint8_t zeros[2] = {0x00, 0x00};
    crc = OSAL_CRC16_CCITT_Update(crc, zeros, OSAL_sizeof(zeros));

    /* 第三段：从 CRC 字段之后到报文结束 */
    crc = OSAL_CRC16_CCITT_Update(crc, packet + crc_offset + 2,
                                   total_len - crc_offset - 2);

    return (crc == received_crc);
}
