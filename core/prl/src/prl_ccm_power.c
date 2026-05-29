/**
 * @file prl_ccm_power.c
 * @brief CCM-Power Protocol Implementation
 */

#include "prl_ccm_power.h"
#include "lib/osal_string.h"

/* ========== 编码函数实现 ========== */

int prl_ccm_power_encode_heartbeat(const prl_ccm_power_heartbeat_t *msg,
                                    uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_heartbeat_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_HEARTBEAT, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_power_encode_power_on(const prl_ccm_power_on_t *msg,
                                   uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_on_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_POWER_ON, payload_len, PRL_FLAG_ACK_REQUIRED);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_power_encode_power_off(const prl_ccm_power_off_t *msg,
                                    uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_off_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_POWER_OFF, payload_len, PRL_FLAG_ACK_REQUIRED);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_power_encode_voltage_query(const prl_ccm_power_voltage_query_t *msg,
                                        uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_voltage_query_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_VOLTAGE_QUERY, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_power_encode_current_query(const prl_ccm_power_current_query_t *msg,
                                        uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_current_query_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_CURRENT_QUERY, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_power_encode_temp_query(const prl_ccm_power_temp_query_t *msg,
                                     uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_temp_query_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_TEMP_QUERY, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_power_encode_status_report(const prl_ccm_power_status_report_t *msg,
                                        uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_status_report_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_STATUS_REPORT, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_power_encode_alarm(const prl_ccm_power_alarm_t *msg,
                                uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_alarm_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_ALARM, payload_len, PRL_FLAG_ACK_REQUIRED);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_power_encode_ack(const prl_ccm_power_ack_t *msg,
                              uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_power_ack_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_PWR_MSG_ACK, payload_len, PRL_FLAG_IS_ACK);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

/* ========== 解码函数实现 ========== */

int prl_ccm_power_decode_heartbeat(const uint8_t *buf, size_t len,
                                    prl_ccm_power_heartbeat_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_heartbeat_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_HEARTBEAT);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_heartbeat_t));

    return PRL_OK;
}

int prl_ccm_power_decode_power_on(const uint8_t *buf, size_t len,
                                   prl_ccm_power_on_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_on_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_POWER_ON);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_on_t));

    return PRL_OK;
}

int prl_ccm_power_decode_power_off(const uint8_t *buf, size_t len,
                                    prl_ccm_power_off_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_off_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_POWER_OFF);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_off_t));

    return PRL_OK;
}

int prl_ccm_power_decode_voltage_query(const uint8_t *buf, size_t len,
                                        prl_ccm_power_voltage_query_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_voltage_query_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_VOLTAGE_QUERY);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_voltage_query_t));

    return PRL_OK;
}

int prl_ccm_power_decode_current_query(const uint8_t *buf, size_t len,
                                        prl_ccm_power_current_query_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_current_query_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_CURRENT_QUERY);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_current_query_t));

    return PRL_OK;
}

int prl_ccm_power_decode_temp_query(const uint8_t *buf, size_t len,
                                     prl_ccm_power_temp_query_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_temp_query_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_TEMP_QUERY);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_temp_query_t));

    return PRL_OK;
}

int prl_ccm_power_decode_status_report(const uint8_t *buf, size_t len,
                                        prl_ccm_power_status_report_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_status_report_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_STATUS_REPORT);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_status_report_t));

    return PRL_OK;
}

int prl_ccm_power_decode_alarm(const uint8_t *buf, size_t len,
                                prl_ccm_power_alarm_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_alarm_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_ALARM);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_alarm_t));

    return PRL_OK;
}

int prl_ccm_power_decode_ack(const uint8_t *buf, size_t len,
                              prl_ccm_power_ack_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_power_ack_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_PWR_MSG_ACK);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_power_ack_t));

    return PRL_OK;
}
