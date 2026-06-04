/**
 * @file pdl_mcu_protocol.c
 * @brief MCU Protocol Wrapper Implementation
 * @details PDL_MCU 协议封装层实现，调用 PRL_DEVICE 统一协议
 */

#include "pdl_mcu_protocol.h"
#include "osal/osal.h"

/* ========== 编码函数 ========== */

int32_t pdl_mcu_encode_get_version(uint8_t *buffer, size_t buffer_size)
{
    /* 版本查询无需负载数据 */
    return prl_device_encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_VERSION,
                             NULL, 0, buffer, buffer_size, 0);
}

int32_t pdl_mcu_encode_get_status(uint8_t *buffer, size_t buffer_size)
{
    /* 状态查询无需负载数据 */
    return prl_device_encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_STATUS,
                             NULL, 0, buffer, buffer_size, 0);
}

int32_t pdl_mcu_encode_reset(uint8_t *buffer, size_t buffer_size)
{
    /* 复位命令无需负载数据 */
    return prl_device_encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_RESET,
                             NULL, 0, buffer, buffer_size, 0);
}

int32_t pdl_mcu_encode_read_register(uint8_t reg_addr, uint8_t *buffer, size_t buffer_size)
{
    /* 负载：寄存器地址 */
    return prl_device_encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_GET_CONFIG,
                             &reg_addr, 1, buffer, buffer_size, 0);
}

int32_t pdl_mcu_encode_write_register(uint8_t reg_addr, uint8_t value,
                                       uint8_t *buffer, size_t buffer_size)
{
    uint8_t data[2];

    /* 负载：寄存器地址 + 值 */
    data[0] = reg_addr;
    data[1] = value;

    return prl_device_encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_SET_CONFIG,
                             data, 2, buffer, buffer_size, 0);
}

int32_t pdl_mcu_encode_custom_command(uint8_t cmd_code, const uint8_t *data,
                                       uint32_t data_len, uint8_t *buffer,
                                       size_t buffer_size)
{
    /* 自定义命令使用心跳消息类型，命令码放在负载中 */
    uint8_t payload[256];

    if (data_len > 255) {
        return OSAL_ERR_INVALID_PARAM;
    }

    payload[0] = cmd_code;
    if (data && data_len > 0) {
        OSAL_Memcpy(&payload[1], data, data_len);
    }

    return prl_device_encode(PRL_DEV_TYPE_MCU, PRL_MCU_MSG_HEARTBEAT,
                             payload, data_len + 1, buffer, buffer_size, 0);
}

/* ========== 解码函数 ========== */

int32_t pdl_mcu_decode_get_version(const uint8_t *packet, size_t packet_len,
                                    pdl_mcu_version_t *version)
{
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret;

    if (!packet || !version) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 解码报文 */
    ret = prl_device_decode(packet, packet_len, &dev_type, &msg_type,
                            &payload, &payload_len);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 验证设备类型和消息类型 */
    if (dev_type != PRL_DEV_TYPE_MCU || msg_type != PRL_MCU_MSG_GET_VERSION) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 解析版本信息：[major][minor][patch][build] */
    if (payload_len < 4) {
        return OSAL_ERR_INVALID_PARAM;
    }

    version->major = payload[0];
    version->minor = payload[1];
    version->patch = payload[2];
    version->build = payload[3];

    OSAL_Snprintf(version->version_string, sizeof(version->version_string),
                  "%d.%d.%d.%d", version->major, version->minor,
                  version->patch, version->build);

    return OSAL_SUCCESS;
}

int32_t pdl_mcu_decode_get_status(const uint8_t *packet, size_t packet_len,
                                   pdl_mcu_status_t *status)
{
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret;

    if (!packet || !status) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 解码报文 */
    ret = prl_device_decode(packet, packet_len, &dev_type, &msg_type,
                            &payload, &payload_len);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 验证设备类型和消息类型 */
    if (dev_type != PRL_DEV_TYPE_MCU || msg_type != PRL_MCU_MSG_GET_STATUS) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 解析状态信息：[uptime(4)][error_code][temp][voltage(2)] */
    if (payload_len < 8) {
        return OSAL_ERR_INVALID_PARAM;
    }

    status->online = true;
    status->uptime_sec = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) | payload[3];
    status->error_code = payload[4];
    status->temperature = (float)payload[5];
    status->voltage_mv = ((uint16_t)payload[6] << 8) | payload[7];
    status->timestamp_us = 0;  /* 时间戳由调用者填充 */

    return OSAL_SUCCESS;
}

int32_t pdl_mcu_decode_read_register(const uint8_t *packet, size_t packet_len,
                                      uint8_t *value)
{
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret;

    if (!packet || !value) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 解码报文 */
    ret = prl_device_decode(packet, packet_len, &dev_type, &msg_type,
                            &payload, &payload_len);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 验证设备类型和消息类型 */
    if (dev_type != PRL_DEV_TYPE_MCU || msg_type != PRL_MCU_MSG_GET_CONFIG) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 解析寄存器值 */
    if (payload_len < 1) {
        return OSAL_ERR_INVALID_PARAM;
    }

    *value = payload[0];

    return OSAL_SUCCESS;
}

int32_t pdl_mcu_decode_response(const uint8_t *packet, size_t packet_len,
                                 uint8_t *response, uint32_t resp_size,
                                 uint32_t *actual_size)
{
    uint8_t dev_type, msg_type;
    const uint8_t *payload;
    uint16_t payload_len;
    int ret;

    if (!packet) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 解码报文 */
    ret = prl_device_decode(packet, packet_len, &dev_type, &msg_type,
                            &payload, &payload_len);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 验证设备类型 */
    if (dev_type != PRL_DEV_TYPE_MCU) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 复制负载数据 */
    if (response && payload_len > 0) {
        uint32_t copy_len = (payload_len < resp_size) ? payload_len : resp_size;
        OSAL_Memcpy(response, payload, copy_len);

        if (actual_size) {
            *actual_size = copy_len;
        }
    } else if (actual_size) {
        *actual_size = 0;
    }

    return OSAL_SUCCESS;
}
