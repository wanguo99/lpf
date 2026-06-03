/**
 * @file prl_common.c
 * @brief Protocol Layer Common Implementation
 */

#include "prl/prl_common.h"
#include "sys/osal_clock.h"
#include "lib/osal_heap.h"
#include "lib/osal_string.h"
#include "net/osal_socket.h"  /* for OSAL_htons/htonl/ntohs/ntohl */

/* 全局序列号（非静态，供 prl_api.c 访问） */
uint32_t g_seq_number = 0;

/**
 * @brief CRC16-CCITT 查找表
 */
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t prl_calc_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }

    return crc;
}

uint32_t prl_get_next_seq(void)
{
    return __sync_fetch_and_add(&g_seq_number, 1);
}

uint32_t prl_get_timestamp(void)
{
    OS_time_t time_struct;
    OSAL_GetLocalTime(&time_struct);
    return time_struct.seconds;
}

void prl_init_header(prl_header_t *hdr, uint8_t dev_type, uint8_t msg_type,
                     uint16_t payload_len, uint8_t flags)
{
    OSAL_Memset(hdr, 0, sizeof(prl_header_t));

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
        return PRL_ERR_INVALID_MAGIC;
    }

    if ((hdr->version >> 4) != PRL_VERSION_MAJOR) {
        return PRL_ERR_INVALID_VERSION;
    }

    if (expected_type != 0 && hdr->msg_type != expected_type) {
        return PRL_ERR_INVALID_DEV_TYPE;
    }

    if (length > PRL_MAX_PAYLOAD_SIZE) {
        return PRL_ERR_INVALID_LENGTH;
    }

    return PRL_OK;
}

void prl_set_packet_crc(uint8_t *packet, size_t total_len)
{
    prl_header_t *hdr = (prl_header_t *)packet;

    /* CRC 字段先置 0 */
    hdr->crc16 = 0;

    /* 计算整个报文的 CRC */
    uint16_t crc = prl_calc_crc16(packet, total_len);

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

    /* 第一段：从报文开始到 CRC 字段之前 */
    for (size_t i = 0; i < crc_offset; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ packet[i]) & 0xFF];
    }

    /* 第二段：CRC 字段作为 0x0000 处理 */
    crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ 0x00) & 0xFF];
    crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ 0x00) & 0xFF];

    /* 第三段：从 CRC 字段之后到报文结束 */
    for (size_t i = crc_offset + sizeof(uint16_t); i < total_len; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ packet[i]) & 0xFF];
    }

    return (crc == received_crc);
}
