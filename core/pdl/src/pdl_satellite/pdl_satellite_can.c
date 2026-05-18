/************************************************************************
 * 卫星平台CAN通信实现
 *
 * 职责：
 * - 封装卫星CAN协议（帧格式、消息类型）
 * - CAN帧的收发
 * - 协议解析和封装
 ************************************************************************/

#include "pdl_satellite_internal.h"
#include "hal_can.h"
#include "osal.h"

/**
 * @brief 初始化卫星CAN通信
 */
int32_t satellite_can_init(const char *device, uint32_t bitrate, void **handle)
{
    if (NULL == device || NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 打开CAN设备 */
    hal_can_config_t can_config = {
        .interface = device,
        .baudrate = bitrate,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    hal_can_handle_t can_handle;
    if (OSAL_SUCCESS != HAL_CAN_Init(&can_config, &can_handle))
    {
        return OSAL_ERR_GENERIC;
    }

    *handle = can_handle;
    return OSAL_SUCCESS;
}

/**
 * @brief 反初始化卫星CAN通信
 */
int32_t satellite_can_deinit(void *handle)
{
    if (NULL == handle)
    {
        return OSAL_ERR_GENERIC;
    }

    hal_can_handle_t can_handle = (hal_can_handle_t)handle;
    return HAL_CAN_Deinit(can_handle);
}

/**
 * @brief 接收卫星CAN消息
 */
int32_t satellite_can_recv(void *handle, satellite_can_msg_t *msg, uint32_t timeout_ms)
{
    if (NULL == handle || NULL == msg)
    {
        return OSAL_ERR_GENERIC;
    }

    hal_can_handle_t can_handle = (hal_can_handle_t)handle;
    can_frame_t frame;

    int32_t ret = HAL_CAN_Recv(can_handle, &frame, timeout_ms);
    if (OSAL_SUCCESS != ret)
    {
        return ret;
    }

    /* 解析CAN帧：[msg_type][seq_num][cmd_type][data] */
    if (frame.dlc == 8)
    {
        msg->msg_type = frame.data[0];
        msg->seq_num = frame.data[1];
        msg->cmd_type = frame.data[2];

        /* 使用字节序转换宏确保跨平台兼容性（网络序->主机序） */
        uint32_t data_be = (frame.data[4] << 24) | (frame.data[5] << 16) |
                           (frame.data[6] << 8) | frame.data[7];
        msg->data = OSAL_NTOHL(data_be);
    }
    else
    {
        LOG_WARN("PDL_Satellite", "Invalid telemetry frame DLC: %u", frame.dlc);
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 发送卫星CAN消息
 */
int32_t satellite_can_send(void *handle, const satellite_can_msg_t *msg)
{
    if (NULL == handle || NULL == msg)
    {
        return OSAL_ERR_GENERIC;
    }

    hal_can_handle_t can_handle = (hal_can_handle_t)handle;

    /* 封装CAN帧：[msg_type][seq_num][cmd_type][reserved][data] */
    can_frame_t frame;
    frame.can_id = SATELLITE_CAN_TX_ID;
    frame.dlc = 8;

    frame.data[0] = msg->msg_type;
    frame.data[1] = msg->seq_num;
    frame.data[2] = msg->cmd_type;
    frame.data[3] = 0;  /* 预留 */

    /* 使用字节序转换宏确保跨平台兼容性（主机序->网络序） */
    uint32_t data_be = OSAL_HTONL(msg->data);
    frame.data[4] = (uint8_t)(data_be >> 24);
    frame.data[5] = (uint8_t)(data_be >> 16);
    frame.data[6] = (uint8_t)(data_be >> 8);
    frame.data[7] = (uint8_t)(data_be & 0xFF);

    return HAL_CAN_Send(can_handle, &frame);
}

/**
 * @brief 发送心跳消息
 */
int32_t satellite_can_send_heartbeat(void *handle, uint8_t status)
{
    satellite_can_msg_t msg = {
        .msg_type = CAN_MSG_TYPE_HEARTBEAT,
        .seq_num = 0,
        .cmd_type = status,
        .data = 0
    };

    return satellite_can_send(handle, &msg);
}

/**
 * @brief 发送响应消息
 */
int32_t satellite_can_send_response(void *handle, uint8_t seq_num, uint8_t status, uint32_t result)
{
    satellite_can_msg_t msg = {
        .msg_type = CAN_MSG_TYPE_CMD_RESP,
        .seq_num = seq_num,
        .cmd_type = status,
        .data = result
    };

    return satellite_can_send(handle, &msg);
}
