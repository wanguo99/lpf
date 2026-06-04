/**
 * @file prl_device.c
 * @brief Protocol Layer Device Messages Implementation
 * @details 统一设备协议消息实现
 */

#include "osal/osal.h"

#include "prl/prl_device.h"


/* 设备类型名称映射表 */
static const char *device_type_names[] = {
    "UNKNOWN",      /* 0x00 */
    "MCU",          /* 0x01 */
    "CCM",          /* 0x02 */
    "PMC",          /* 0x03 */
    "GSC",          /* 0x04 */
    "SATELLITE",    /* 0x05 */
    "POWER",        /* 0x06 */
    "BMC",          /* 0x07 */
};

bool prl_device_type_valid(uint8_t dev_type)
{
    return (dev_type >= PRL_DEV_TYPE_MCU && dev_type <= PRL_DEV_TYPE_BMC);
}

const char *prl_device_type_name(uint8_t dev_type)
{
    if (dev_type < sizeof(device_type_names) / sizeof(device_type_names[0])) {
        return device_type_names[dev_type];
    }
    return "INVALID";
}

int prl_device_encode(uint8_t dev_type, uint8_t msg_type,
                      const void *payload, uint16_t payload_len,
                      uint8_t *buffer, size_t buffer_size, uint8_t flags)
{
    prl_header_t *hdr;
    size_t total_len;

    /* 参数检查 */
    if (!buffer) {
        return OSAL_ERR_INVALID_PARAM;
    }

    if (!prl_device_type_valid(dev_type)) {
        return OSAL_EINVAL;
    }

    /* 检查缓冲区大小 */
    total_len = PRL_HEADER_SIZE + payload_len;
    if (buffer_size < total_len) {
        return OSAL_ENOBUFS;
    }

    /* 检查负载长度 */
    if (payload_len > PRL_MAX_PAYLOAD_SIZE) {
        return OSAL_EINVAL;
    }

    /* 初始化协议头 */
    hdr = (prl_header_t *)buffer;
    prl_init_header(hdr, dev_type, msg_type, payload_len, flags);

    /* 复制负载数据 */
    if (payload && payload_len > 0) {
        OSAL_Memcpy(buffer + PRL_HEADER_SIZE, payload, payload_len);
    }

    /* 计算并设置 CRC */
    prl_set_packet_crc(buffer, total_len);

    return (int)total_len;
}

int prl_device_decode(const uint8_t *packet, size_t packet_len,
                      uint8_t *dev_type, uint8_t *msg_type,
                      const uint8_t **payload, uint16_t *payload_len)
{
    const prl_header_t *hdr;
    uint16_t payload_length;  /* 主机字节序的负载长度 */
    int ret;

    /* 参数检查 */
    if (!packet || !dev_type || !msg_type || !payload || !payload_len) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 长度检查 */
    if (packet_len < PRL_HEADER_SIZE) {
        return OSAL_EINVAL;
    }

    hdr = (const prl_header_t *)packet;

    /* 验证协议头 */
    ret = prl_validate_header(hdr, 0);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 验证设备类型 */
    if (!prl_device_type_valid(hdr->dev_type)) {
        return OSAL_EINVAL;
    }

    /* 将网络字节序转换为主机字节序 */
    payload_length = OSAL_ntohs(hdr->length);

    /* 验证报文长度 */
    if (packet_len < (PRL_HEADER_SIZE + payload_length)) {
        return OSAL_EINVAL;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(packet, PRL_HEADER_SIZE + payload_length)) {
        return OSAL_EBADMSG;
    }

    /* 提取信息 */
    *dev_type = hdr->dev_type;
    *msg_type = hdr->msg_type;
    *payload_len = payload_length;

    if (payload_length > 0) {
        *payload = packet + PRL_HEADER_SIZE;
    } else {
        *payload = NULL;
    }

    return OSAL_SUCCESS;
}
