/**
 * @file prl_ccm_satellite.c
 * @brief CCM-Satellite Protocol Implementation
 */

#include "prl_ccm_satellite.h"
#include "lib/osal_string.h"

/* ========== 编码函数实现 ========== */

int prl_ccm_satellite_encode_heartbeat(const prl_ccm_satellite_heartbeat_t *msg,
                                        uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_heartbeat_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_HEARTBEAT, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_satellite_encode_telemetry(const prl_ccm_satellite_telemetry_t *msg,
                                        const uint8_t *data, size_t data_len,
                                        uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (data_len > 0 && !data) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_telemetry_t) + data_len;
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_TELEMETRY, payload_len, 0);

    uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(payload, msg, sizeof(prl_ccm_satellite_telemetry_t));

    if (data_len > 0) {
        OSAL_Memcpy(payload + sizeof(prl_ccm_satellite_telemetry_t), data, data_len);
    }

    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_satellite_encode_telecommand(const prl_ccm_satellite_telecommand_t *msg,
                                          const uint8_t *params, size_t params_len,
                                          uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (params_len > 0 && !params) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_telecommand_t) + params_len;
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_TELECOMMAND, payload_len, PRL_FLAG_ACK_REQUIRED);

    uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(payload, msg, sizeof(prl_ccm_satellite_telecommand_t));

    if (params_len > 0) {
        OSAL_Memcpy(payload + sizeof(prl_ccm_satellite_telecommand_t), params, params_len);
    }

    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_satellite_encode_time_sync(const prl_ccm_satellite_time_sync_t *msg,
                                        uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_time_sync_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_TIME_SYNC, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_satellite_encode_orbit_data(const prl_ccm_satellite_orbit_data_t *msg,
                                         uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_orbit_data_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_ORBIT_DATA, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_satellite_encode_attitude_data(const prl_ccm_satellite_attitude_data_t *msg,
                                            uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_attitude_data_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_ATTITUDE_DATA, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_satellite_encode_power_status(const prl_ccm_satellite_power_status_t *msg,
                                           uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_power_status_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_POWER_STATUS, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_satellite_encode_thermal_status(const prl_ccm_satellite_thermal_status_t *msg,
                                             uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_thermal_status_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_THERMAL_STATUS, payload_len, 0);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

int prl_ccm_satellite_encode_ack(const prl_ccm_satellite_ack_t *msg,
                                  uint8_t *buf, size_t *len)
{
    if (!msg || !buf || !len) {
        return PRL_ERR_INVALID_PARAM;
    }

    size_t payload_len = sizeof(prl_ccm_satellite_ack_t);
    size_t total_len = PRL_HEADER_SIZE + payload_len;

    if (*len < total_len) {
        return PRL_ERR_BUFFER_TOO_SMALL;
    }

    prl_header_t *hdr = (prl_header_t *)buf;
    prl_init_header(hdr, PRL_CCM_SAT_MSG_ACK, payload_len, PRL_FLAG_IS_ACK);

    OSAL_Memcpy(buf + PRL_HEADER_SIZE, msg, payload_len);
    prl_set_packet_crc(buf, total_len);

    *len = total_len;
    return PRL_OK;
}

/* ========== 解码函数实现 ========== */

int prl_ccm_satellite_decode_heartbeat(const uint8_t *buf, size_t len,
                                        prl_ccm_satellite_heartbeat_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_heartbeat_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_HEARTBEAT);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_heartbeat_t));

    return PRL_OK;
}

int prl_ccm_satellite_decode_telemetry(const uint8_t *buf, size_t len,
                                        prl_ccm_satellite_telemetry_t *msg,
                                        uint8_t **data, size_t *data_len)
{
    if (!buf || !msg || !data || !data_len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_telemetry_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_TELEMETRY);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_telemetry_t));

    *data_len = hdr->length - sizeof(prl_ccm_satellite_telemetry_t);
    if (*data_len > 0) {
        *data = (uint8_t *)(payload + sizeof(prl_ccm_satellite_telemetry_t));
    } else {
        *data = NULL;
    }

    return PRL_OK;
}

int prl_ccm_satellite_decode_telecommand(const uint8_t *buf, size_t len,
                                          prl_ccm_satellite_telecommand_t *msg,
                                          uint8_t **params, size_t *params_len)
{
    if (!buf || !msg || !params || !params_len) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_telecommand_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_TELECOMMAND);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_telecommand_t));

    *params_len = hdr->length - sizeof(prl_ccm_satellite_telecommand_t);
    if (*params_len > 0) {
        *params = (uint8_t *)(payload + sizeof(prl_ccm_satellite_telecommand_t));
    } else {
        *params = NULL;
    }

    return PRL_OK;
}

int prl_ccm_satellite_decode_time_sync(const uint8_t *buf, size_t len,
                                        prl_ccm_satellite_time_sync_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_time_sync_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_TIME_SYNC);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_time_sync_t));

    return PRL_OK;
}

int prl_ccm_satellite_decode_orbit_data(const uint8_t *buf, size_t len,
                                         prl_ccm_satellite_orbit_data_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_orbit_data_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_ORBIT_DATA);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_orbit_data_t));

    return PRL_OK;
}

int prl_ccm_satellite_decode_attitude_data(const uint8_t *buf, size_t len,
                                            prl_ccm_satellite_attitude_data_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_attitude_data_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_ATTITUDE_DATA);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_attitude_data_t));

    return PRL_OK;
}

int prl_ccm_satellite_decode_power_status(const uint8_t *buf, size_t len,
                                           prl_ccm_satellite_power_status_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_power_status_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_POWER_STATUS);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_power_status_t));

    return PRL_OK;
}

int prl_ccm_satellite_decode_thermal_status(const uint8_t *buf, size_t len,
                                             prl_ccm_satellite_thermal_status_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_thermal_status_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_THERMAL_STATUS);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_thermal_status_t));

    return PRL_OK;
}

int prl_ccm_satellite_decode_ack(const uint8_t *buf, size_t len,
                                  prl_ccm_satellite_ack_t *msg)
{
    if (!buf || !msg) {
        return PRL_ERR_INVALID_PARAM;
    }

    if (len < PRL_HEADER_SIZE + sizeof(prl_ccm_satellite_ack_t)) {
        return PRL_ERR_INVALID_LENGTH;
    }

    if (!prl_verify_packet_crc(buf, len)) {
        return PRL_ERR_CRC_FAILED;
    }

    const prl_header_t *hdr = (const prl_header_t *)buf;
    int ret = prl_validate_header(hdr, PRL_CCM_SAT_MSG_ACK);
    if (ret != PRL_OK) {
        return ret;
    }

    const uint8_t *payload = buf + PRL_HEADER_SIZE;
    OSAL_Memcpy(msg, payload, sizeof(prl_ccm_satellite_ack_t));

    return PRL_OK;
}
