/************************************************************************
 * 卫星平台服务内部接口
 *
 * 仅供pdl_satellite模块内部使用，不对外暴露
 ************************************************************************/

#ifndef PDL_SATELLITE_INTERNAL_H
#define PDL_SATELLITE_INTERNAL_H

#include "osal.h"
#include "hal.h"
#include "pconfig.h"
#include "pdl_satellite.h"

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
 * 卫星消息结构（CAN 和 Ethernet 通用）
 */
typedef struct
{
    uint8_t msg_type;      /* 消息类型 */
    uint8_t seq_num;       /* 序列号 */
    uint8_t cmd_type;      /* 命令类型/状态码 */
    uint32_t data;         /* 数据 */
} satellite_can_msg_t;

/*
 * Satellite 通信接口操作函数表（类似 Linux 驱动的 ops 结构）
 *
 * 设计理念：
 * - 在初始化时根据接口类型注册 ops（只执行一次 switch 判断）
 * - 运行时直接通过函数指针调用，无需重复判断接口类型
 * - 符合 Linux 内核驱动设计模式
 */
typedef struct pdl_satellite_ops {
	/**
	 * @brief 初始化通信接口
	 * @param[in] config 接口配置
	 * @param[out] handle 返回通信句柄
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*init)(const void *config, void **handle);

	/**
	 * @brief 反初始化通信接口
	 * @param[in] handle 通信句柄
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*deinit)(void *handle);

	/**
	 * @brief 接收消息
	 * @param[in] handle 通信句柄
	 * @param[out] msg 消息结构
	 * @param[in] timeout_ms 超时时间（毫秒）
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*recv)(void *handle, satellite_can_msg_t *msg, uint32_t timeout_ms);

	/**
	 * @brief 发送消息
	 * @param[in] handle 通信句柄
	 * @param[in] msg 消息结构
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*send)(void *handle, const satellite_can_msg_t *msg);

	/**
	 * @brief 发送心跳
	 * @param[in] handle 通信句柄
	 * @param[in] status 状态码
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*send_heartbeat)(void *handle, uint8_t status);

	/**
	 * @brief 发送响应
	 * @param[in] handle 通信句柄
	 * @param[in] seq_num 序列号
	 * @param[in] status 状态码
	 * @param[in] result 结果数据
	 * @return OSAL_SUCCESS 成功
	 */
	int32_t (*send_response)(void *handle, uint8_t seq_num, uint8_t status, uint32_t result);
} pdl_satellite_ops_t;

/*
 * CAN 通信接口（pdl_satellite_can.c 实现）
 */
int32_t satellite_can_init(const char *device, uint32_t bitrate, void **handle);
int32_t satellite_can_deinit(void *handle);
int32_t satellite_can_recv(void *handle, satellite_can_msg_t *msg, uint32_t timeout_ms);
int32_t satellite_can_send(void *handle, const satellite_can_msg_t *msg);
int32_t satellite_can_send_heartbeat(void *handle, uint8_t status);
int32_t satellite_can_send_response(void *handle, uint8_t seq_num, uint8_t status, uint32_t result);

/* CAN 接口的 ops 结构（在 pdl_satellite_can.c 中定义） */
extern const pdl_satellite_ops_t satellite_can_ops;

/*
 * Ethernet 通信接口（pdl_satellite_ethernet.c 实现）
 */
/* Ethernet 接口的 ops 结构（在 pdl_satellite_ethernet.c 中定义） */
extern const pdl_satellite_ops_t satellite_ethernet_ops;

#endif /* PDL_SATELLITE_INTERNAL_H */
