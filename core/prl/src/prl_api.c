/**
 * @file prl_api.c
 * @brief Protocol Layer Public API Implementation
 * @details PRL 协议层对外 API 实现
 */

#include "osal.h"

#include "prl.h"

/* 全局初始化标志 */
static bool g_prl_initialized = false;

int PRL_Init(void)
{
    if (g_prl_initialized) {
        return OSAL_SUCCESS;
    }

    /* 初始化序列号（可选） */
    /* 这里可以从持久化存储中恢复序列号 */

    g_prl_initialized = true;
    return OSAL_SUCCESS;
}

int PRL_Deinit(void)
{
    if (!g_prl_initialized) {
        return OSAL_SUCCESS;
    }

    /* 清理资源（如果有） */

    g_prl_initialized = false;
    return OSAL_SUCCESS;
}

int PRL_Encode(uint8_t dev_type, uint8_t msg_type,
               const void *payload, uint16_t payload_len,
               uint8_t *buffer, size_t buffer_size, uint8_t flags)
{
    /* 直接调用内部实现 */
    return prl_device_encode(dev_type, msg_type, payload, payload_len,
                             buffer, buffer_size, flags);
}

int PRL_Decode(const uint8_t *packet, size_t packet_len,
               uint8_t *dev_type, uint8_t *msg_type,
               const uint8_t **payload, uint16_t *payload_len)
{
    /* 直接调用内部实现 */
    return prl_device_decode(packet, packet_len, dev_type, msg_type,
                             payload, payload_len);
}

bool PRL_IsDeviceTypeValid(uint8_t dev_type)
{
    return prl_device_type_valid(dev_type);
}

const char *PRL_GetDeviceTypeName(uint8_t dev_type)
{
    return prl_device_type_name(dev_type);
}

const char *PRL_GetErrorString(int error_code)
{
    /* 现在使用 OSAL 错误码，直接返回 OSAL 错误描述 */
    return OSAL_get_status_name(error_code);
}

void PRL_GetVersion(uint8_t *major, uint8_t *minor)
{
    if (major) {
        *major = PRL_VERSION_MAJOR;
    }
    if (minor) {
        *minor = PRL_VERSION_MINOR;
    }
}

void PRL_ResetSequence(uint32_t seq)
{
    extern osal_atomic_uint32_t g_seq_number;  /* 定义在 prl_common.c */
    OSAL_AtomicStore(&g_seq_number, seq);
}

uint32_t PRL_GetCurrentSequence(void)
{
    extern osal_atomic_uint32_t g_seq_number;  /* 定义在 prl_common.c */
    return OSAL_AtomicLoad(&g_seq_number);
}

int PRL_ValidatePacket(const uint8_t *packet, size_t packet_len)
{
    const prl_header_t *hdr;

    /* 参数检查 */
    if (!packet) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 长度检查 */
    if (packet_len < PRL_HEADER_SIZE) {
        return OSAL_EINVAL;
    }

    hdr = (const prl_header_t *)packet;

    /* 验证协议头 */
    int ret = prl_validate_header(hdr, 0);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 验证设备类型 */
    if (!prl_device_type_valid(hdr->dev_type)) {
        return OSAL_EINVAL;
    }

    /* 验证报文长度 */
    if (packet_len < (PRL_HEADER_SIZE + hdr->length)) {
        return OSAL_EINVAL;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(packet, PRL_HEADER_SIZE + hdr->length)) {
        return OSAL_EBADMSG;
    }

    return OSAL_SUCCESS;
}

int PRL_GetDeviceType(const uint8_t *packet, size_t packet_len,
                      uint8_t *dev_type)
{
    const prl_header_t *hdr;

    /* 参数检查 */
    if (!packet || !dev_type) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 长度检查 */
    if (packet_len < PRL_HEADER_SIZE) {
        return OSAL_EINVAL;
    }

    hdr = (const prl_header_t *)packet;

    /* 提取设备类型 */
    *dev_type = hdr->dev_type;

    return OSAL_SUCCESS;
}

int PRL_GetMessageType(const uint8_t *packet, size_t packet_len,
                       uint8_t *msg_type)
{
    const prl_header_t *hdr;

    /* 参数检查 */
    if (!packet || !msg_type) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 长度检查 */
    if (packet_len < PRL_HEADER_SIZE) {
        return OSAL_EINVAL;
    }

    hdr = (const prl_header_t *)packet;

    /* 提取消息类型 */
    *msg_type = hdr->msg_type;

    return OSAL_SUCCESS;
}

int PRL_GetSequence(const uint8_t *packet, size_t packet_len,
                    uint32_t *seq)
{
    const prl_header_t *hdr;

    /* 参数检查 */
    if (!packet || !seq) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 长度检查 */
    if (packet_len < PRL_HEADER_SIZE) {
        return OSAL_EINVAL;
    }

    hdr = (const prl_header_t *)packet;

    /* 提取序列号 */
    *seq = hdr->seq;

    return OSAL_SUCCESS;
}

int PRL_BuildResponse(const uint8_t *request_packet, size_t request_len,
                      const void *response_payload, uint16_t response_payload_len,
                      uint8_t *response_buffer, size_t response_buffer_size)
{
    const prl_header_t *req_hdr;
    prl_header_t *resp_hdr;
    size_t total_len;

    /* 参数检查 */
    if (!request_packet || !response_buffer) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 长度检查 */
    if (request_len < PRL_HEADER_SIZE) {
        return OSAL_EINVAL;
    }

    req_hdr = (const prl_header_t *)request_packet;

    /* 验证请求报文 */
    int ret = prl_validate_header(req_hdr, 0);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 检查响应缓冲区大小 */
    total_len = PRL_HEADER_SIZE + response_payload_len;
    if (response_buffer_size < total_len) {
        return OSAL_ENOBUFS;
    }

    /* 初始化响应协议头 */
    resp_hdr = (prl_header_t *)response_buffer;
    prl_init_header(resp_hdr, req_hdr->dev_type, req_hdr->msg_type,
                    response_payload_len, PRL_FLAG_IS_ACK);

    /* 使用请求报文的序列号 */
    resp_hdr->seq = req_hdr->seq;

    /* 复制响应负载 */
    if (response_payload && response_payload_len > 0) {
        OSAL_memcpy(response_buffer + PRL_HEADER_SIZE,
                    response_payload, response_payload_len);
    }

    /* 计算并设置 CRC */
    prl_set_packet_crc(response_buffer, total_len);

    return (int)total_len;
}
