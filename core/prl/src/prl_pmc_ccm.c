/**
 * @file prl_pmc_ccm.c
 * @brief PMC-CCM Protocol Implementation
 */

#include "prl_pmc_ccm.h"
#include "lib/osal_string.h"

/* ========== 编码函数实现 ========== */

int prl_pmc_ccm_encode_heartbeat(const prl_pmc_ccm_heartbeat_t *msg,
                                  uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_pmc_ccm_heartbeat_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_PMC_CCM_MSG_HEARTBEAT, payload_len, 0);

    /* 拷贝消息体 */
    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_pmc_ccm_encode_telemetry(const prl_pmc_ccm_telemetry_t *msg,
                                  const uint8_t *data, size_t data_len,
                                  uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (data_len > 0 && !data) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_pmc_ccm_telemetry_t) + data_len;
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_PMC_CCM_MSG_TELEMETRY, payload_len, 0);

    /* 拷贝消息体 */
    uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(payload, msg, sizeof(prl_pmc_ccm_telemetry_t));

    /* 拷贝变长数据 */
    if (data_len > 0) {
        OSAL_Memcpy(payload + sizeof(prl_pmc_ccm_telemetry_t), data, data_len);
    }

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_pmc_ccm_encode_command(const prl_pmc_ccm_command_t *msg,
                                const uint8_t *params, size_t params_len,
                                uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (params_len > 0 && !params) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_pmc_ccm_command_t) + params_len;
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_PMC_CCM_MSG_COMMAND, payload_len, PRL_FLAG_ACK_REQUIRED);

    /* 拷贝消息体 */
    uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(payload, msg, sizeof(prl_pmc_ccm_command_t));

    /* 拷贝参数数据 */
    if (params_len > 0) {
        OSAL_Memcpy(payload + sizeof(prl_pmc_ccm_command_t), params, params_len);
    }

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_pmc_ccm_encode_firmware_update(const prl_pmc_ccm_firmware_update_t *msg,
                                        const uint8_t *data, size_t data_len,
                                        uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (data_len > 0 && !data) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_pmc_ccm_firmware_update_t) + data_len;
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_PMC_CCM_MSG_FIRMWARE_UPDATE, payload_len, PRL_FLAG_ACK_REQUIRED);

    /* 拷贝消息体 */
    uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(payload, msg, sizeof(prl_pmc_ccm_firmware_update_t));

    /* 拷贝固件数据 */
    if (data_len > 0) {
        OSAL_Memcpy(payload + sizeof(prl_pmc_ccm_firmware_update_t), data, data_len);
    }

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_pmc_ccm_encode_node_manage(const prl_pmc_ccm_node_manage_t *msg,
                                    uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_pmc_ccm_node_manage_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_PMC_CCM_MSG_NODE_MANAGE, payload_len, PRL_FLAG_ACK_REQUIRED);

    /* 拷贝消息体 */
    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_pmc_ccm_encode_power_control(const prl_pmc_ccm_power_control_t *msg,
                                      uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_pmc_ccm_power_control_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_PMC_CCM_MSG_POWER_CONTROL, payload_len, PRL_FLAG_ACK_REQUIRED);

    /* 拷贝消息体 */
    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_pmc_ccm_encode_status_query(const prl_pmc_ccm_status_query_t *msg,
                                     uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_pmc_ccm_status_query_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_PMC_CCM_MSG_STATUS_QUERY, payload_len, PRL_FLAG_ACK_REQUIRED);

    /* 拷贝消息体 */
    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_pmc_ccm_encode_ack(const prl_pmc_ccm_ack_t *msg,
                            const uint8_t *data, size_t data_len,
                            uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (data_len > 0 && !data) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_pmc_ccm_ack_t) + data_len;
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    /* 初始化协议头 */
    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_PMC_CCM_MSG_ACK, payload_len, PRL_FLAG_IS_ACK);

    /* 拷贝消息体 */
    uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(payload, msg, sizeof(prl_pmc_ccm_ack_t));

    /* 拷贝附加数据 */
    if (data_len > 0) {
        OSAL_Memcpy(payload + sizeof(prl_pmc_ccm_ack_t), data, data_len);
    }

    /* 计算并设置 CRC */
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

/* ========== 解码函数实现 ========== */

int prl_pmc_ccm_decode_heartbeat(const uint8_t *buf, size_t len,
                                  prl_pmc_ccm_heartbeat_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_pmc_ccm_heartbeat_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_PMC_CCM_MSG_HEARTBEAT);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_pmc_ccm_heartbeat_t));

    return PRL_OK;
}

int prl_pmc_ccm_decode_telemetry(const uint8_t *buf, size_t len,
                                  prl_pmc_ccm_telemetry_t *msg,
                                  uint8_t **data, size_t *data_len)
{
    if (!buf || !msg || !data || !data_len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_pmc_ccm_telemetry_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_PMC_CCM_MSG_TELEMETRY);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_pmc_ccm_telemetry_t));

    /* 解析变长数据 */
    *data_len = hdr->length - sizeof(prl_pmc_ccm_telemetry_t);
    if (*data_len > 0) {
        *data = (uint8_t *)(payload + sizeof(prl_pmc_ccm_telemetry_t));
    } else {
        *data = NULL;
    }

    return PRL_OK;
}

int prl_pmc_ccm_decode_command(const uint8_t *buf, size_t len,
                                prl_pmc_ccm_command_t *msg,
                                uint8_t **params, size_t *params_len)
{
    if (!buf || !msg || !params || !params_len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_pmc_ccm_command_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_PMC_CCM_MSG_COMMAND);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_pmc_ccm_command_t));

    /* 解析参数数据 */
    *params_len = hdr->length - sizeof(prl_pmc_ccm_command_t);
    if (*params_len > 0) {
        *params = (uint8_t *)(payload + sizeof(prl_pmc_ccm_command_t));
    } else {
        *params = NULL;
    }

    return PRL_OK;
}

int prl_pmc_ccm_decode_firmware_update(const uint8_t *buf, size_t len,
                                        prl_pmc_ccm_firmware_update_t *msg,
                                        uint8_t **data, size_t *data_len)
{
    if (!buf || !msg || !data || !data_len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_pmc_ccm_firmware_update_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_PMC_CCM_MSG_FIRMWARE_UPDATE);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_pmc_ccm_firmware_update_t));

    /* 解析固件数据 */
    *data_len = hdr->length - sizeof(prl_pmc_ccm_firmware_update_t);
    if (*data_len > 0) {
        *data = (uint8_t *)(payload + sizeof(prl_pmc_ccm_firmware_update_t));
    } else {
        *data = NULL;
    }

    return PRL_OK;
}

int prl_pmc_ccm_decode_node_manage(const uint8_t *buf, size_t len,
                                    prl_pmc_ccm_node_manage_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_pmc_ccm_node_manage_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_PMC_CCM_MSG_NODE_MANAGE);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_pmc_ccm_node_manage_t));

    return PRL_OK;
}

int prl_pmc_ccm_decode_power_control(const uint8_t *buf, size_t len,
                                      prl_pmc_ccm_power_control_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_pmc_ccm_power_control_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_PMC_CCM_MSG_POWER_CONTROL);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_pmc_ccm_power_control_t));

    return PRL_OK;
}

int prl_pmc_ccm_decode_status_query(const uint8_t *buf, size_t len,
                                     prl_pmc_ccm_status_query_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_pmc_ccm_status_query_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_PMC_CCM_MSG_STATUS_QUERY);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_pmc_ccm_status_query_t));

    return PRL_OK;
}

int prl_pmc_ccm_decode_ack(const uint8_t *buf, size_t len,
                            prl_pmc_ccm_ack_t *msg,
                            uint8_t **data, size_t *data_len)
{
    if (!buf || !msg || !data || !data_len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_pmc_ccm_ack_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    /* 验证 CRC */
    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    /* 验证协议头 */
    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_PMC_CCM_MSG_ACK);
    if (ret != PRL_OK) {
        return ret;
    }

    /* 解析消息体 */
    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_pmc_ccm_ack_t));

    /* 解析附加数据 */
    *data_len = hdr->length - sizeof(prl_pmc_ccm_ack_t);
    if (*data_len > 0) {
        *data = (uint8_t *)(payload + sizeof(prl_pmc_ccm_ack_t));
    } else {
        *data = NULL;
    }

    return PRL_OK;
}
