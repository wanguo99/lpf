/**
 * @file prl_gsc_pmc.c
 * @brief GSC-PMC Protocol Implementation
 */

#include "prl_gsc_pmc.h"
#include "lib/osal_string.h"

/* ========== 编码函数实现 ========== */

int prl_gsc_pmc_encode_heartbeat(const prl_gsc_pmc_heartbeat_t *msg,
                                  uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_gsc_pmc_heartbeat_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_GSC_PMC_MSG_HEARTBEAT, payload_len, 0);

    /* 拷贝消息体 */
    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_gsc_pmc_encode_telemetry(const prl_gsc_pmc_telemetry_t *msg,
                                  const uint8_t *data, size_t data_len,
                                  uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (data_len > 0 && !data) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_gsc_pmc_telemetry_t) + data_len;
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_GSC_PMC_MSG_TELEMETRY, payload_len, 0);

    /* 拷贝消息体 */
    uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(payload, msg, sizeof(prl_gsc_pmc_telemetry_t));

    /* 拷贝遥测数据 */
    if (data_len > 0) {
        OSAL_Memcpy(payload + sizeof(prl_gsc_pmc_telemetry_t), data, data_len);
    }

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_gsc_pmc_encode_command(const prl_gsc_pmc_command_t *msg,
                                const uint8_t *params, size_t params_len,
                                uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (params_len > 0 && !params) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_gsc_pmc_command_t) + params_len;
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_GSC_PMC_MSG_COMMAND, payload_len, PRL_FLAG_ACK_REQUIRED);

    /* 拷贝消息体 */
    uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(payload, msg, sizeof(prl_gsc_pmc_command_t));

    /* 拷贝参数数据 */
    if (params_len > 0) {
        OSAL_Memcpy(payload + sizeof(prl_gsc_pmc_command_t), params, params_len);
    }

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_gsc_pmc_encode_ack(const prl_gsc_pmc_ack_t *msg,
                            uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_gsc_pmc_ack_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_GSC_PMC_MSG_ACK, payload_len, PRL_FLAG_IS_ACK);

    /* 拷贝消息体 */
    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

/* ========== 解码函数实现 ========== */

int prl_gsc_pmc_decode_heartbeat(const uint8_t *buf, size_t len,
                                  prl_gsc_pmc_heartbeat_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_gsc_pmc_heartbeat_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_GSC_PMC_MSG_HEARTBEAT);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_gsc_pmc_heartbeat_t));

    return PRL_OK;
}

int prl_gsc_pmc_decode_telemetry(const uint8_t *buf, size_t len,
                                  prl_gsc_pmc_telemetry_t *msg,
                                  uint8_t **data, size_t *data_len)
{
    if (!buf || !msg || !data || !data_len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_gsc_pmc_telemetry_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_GSC_PMC_MSG_TELEMETRY);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_gsc_pmc_telemetry_t));

    /* 解析遥测数据 */
    *data_len = hdr->length - sizeof(prl_gsc_pmc_telemetry_t);
    if (*data_len > 0) {
        *data = (uint8_t *)(payload + sizeof(prl_gsc_pmc_telemetry_t));
    } else {
        *data = NULL;
    }

    return PRL_OK;
}

int prl_gsc_pmc_decode_command(const uint8_t *buf, size_t len,
                                prl_gsc_pmc_command_t *msg,
                                uint8_t **params, size_t *params_len)
{
    if (!buf || !msg || !params || !params_len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_gsc_pmc_command_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_GSC_PMC_MSG_COMMAND);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_gsc_pmc_command_t));

    /* 解析参数数据 */
    *params_len = hdr->length - sizeof(prl_gsc_pmc_command_t);
    if (*params_len > 0) {
        *params = (uint8_t *)(payload + sizeof(prl_gsc_pmc_command_t));
    } else {
        *params = NULL;
    }

    return PRL_OK;
}

int prl_gsc_pmc_decode_ack(const uint8_t *buf, size_t len,
                            prl_gsc_pmc_ack_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_gsc_pmc_ack_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_GSC_PMC_MSG_ACK);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_gsc_pmc_ack_t));

    return PRL_OK;
}
