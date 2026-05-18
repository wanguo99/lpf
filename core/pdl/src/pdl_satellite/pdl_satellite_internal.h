/************************************************************************
 * 卫星平台服务内部接口
 *
 * 仅供pdl_satellite模块内部使用，不对外暴露
 ************************************************************************/

#ifndef PDL_SATELLITE_INTERNAL_H
#define PDL_SATELLITE_INTERNAL_H

#include "osal_types.h"
#include "pdl_satellite.h"  /* 引入公共头文件，使用其中的can_status_t定义 */

/*
 * 卫星CAN ID定义
 */
#define SATELLITE_CAN_RX_ID   0x100   /* 接收ID（卫星→管理板） */
#define SATELLITE_CAN_TX_ID   0x101   /* 发送ID（管理板→卫星） */

/*
 * CAN消息类型
 */
#define CAN_MSG_TYPE_CMD_REQ      0x01   /* 命令请求 */
#define CAN_MSG_TYPE_CMD_RESP     0x02   /* 命令响应 */
#define CAN_MSG_TYPE_HEARTBEAT    0x03   /* 心跳 */

/*
 * 卫星CAN消息结构
 */
typedef struct
{
    uint8_t msg_type;      /* 消息类型 */
    uint8_t seq_num;       /* 序列号 */
    uint8_t cmd_type;      /* 命令类型/状态码 */
    uint32_t data;         /* 数据 */
} satellite_can_msg_t;

/*
 * CAN通信接口（pdl_satellite_can.c实现）
 */
int32_t satellite_can_init(const char *device, uint32_t bitrate, void **handle);
int32_t satellite_can_deinit(void *handle);
int32_t satellite_can_recv(void *handle, satellite_can_msg_t *msg, uint32_t timeout_ms);
int32_t satellite_can_send(void *handle, const satellite_can_msg_t *msg);
int32_t satellite_can_send_heartbeat(void *handle, uint8_t status);
int32_t satellite_can_send_response(void *handle, uint8_t seq_num, uint8_t status, uint32_t result);

#endif /* PDL_SATELLITE_INTERNAL_H */
