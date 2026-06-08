/************************************************************************
 * HAL层 - CAN总线硬件抽象层API
 *
 * 提供统一的CAN总线访问接口（基于Linux SocketCAN）
 ************************************************************************/

#ifndef HAL_CAN_H
#define HAL_CAN_H

#include "config/hal_can_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 类型定义
 *===========================================================================*/

/**
 * @brief CAN设备句柄（不透明指针）
 */
typedef void* hal_can_handle_t;

/**
 * @brief CAN配置结构
 */
typedef struct
{
	const char *interface;  /* CAN接口名，如 "can0", "vcan0" */
	uint32_t    baudrate;   /* 波特率（Hz） */
	uint32_t    rx_timeout; /* 接收超时（ms） */
	uint32_t    tx_timeout; /* 发送超时（ms） */
} hal_can_config_t;

/*===========================================================================
 * API函数
 *===========================================================================*/

/**
 * @brief 初始化CAN驱动
 *
 * 打开指定的CAN接口并配置参数。该函数会创建SocketCAN套接字，
 * 绑定到指定的CAN接口，并应用配置的超时参数。
 *
 * @param[in]  config  CAN配置参数
 * @param[out] handle  返回的CAN句柄（用于后续操作）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效（config或handle为NULL）
 * @return OSAL_ERR_NO_DEVICE     CAN接口不存在
 * @return OSAL_ERR_GENERIC       其他错误（如权限不足、资源不足）
 *
 * @note 调用此函数前，确保CAN接口已配置并up（如 ip link set can0 up）
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle);

/**
 * @brief 关闭CAN驱动
 *
 * 释放CAN资源，关闭套接字，释放内存。
 *
 * @param[in] handle CAN句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_CAN_Deinit(hal_can_handle_t handle);

/**
 * @brief 发送CAN帧
 *
 * 发送一个标准或扩展CAN帧到总线。
 *
 * @param[in] handle CAN句柄
 * @param[in] frame  要发送的CAN帧
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效（handle或frame为NULL）
 * @return OSAL_ERR_TIMEOUT       发送超时
 * @return OSAL_ERR_GENERIC       发送失败（如总线错误、缓冲区满）
 *
 * @note 发送操作使用配置的tx_timeout作为超时时间
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_CAN_Send(hal_can_handle_t handle, const hal_can_frame_t *frame);

/**
 * @brief 接收CAN帧
 *
 * 从CAN总线接收一个帧。
 *
 * @param[in]  handle  CAN句柄
 * @param[out] frame   接收到的CAN帧
 * @param[in]  timeout 超时时间（ms），OS_PEND表示永久等待
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效（handle或frame为NULL）
 * @return OSAL_ERR_TIMEOUT       接收超时
 * @return OSAL_ERR_GENERIC       接收失败（如总线错误）
 *
 * @note 如果timeout=OS_PEND，则阻塞等待直到收到帧
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_CAN_Recv(hal_can_handle_t handle, hal_can_frame_t *frame, int32_t timeout);

/**
 * @brief 设置CAN过滤器
 *
 * 配置硬件CAN过滤器，只接收匹配的CAN ID。
 *
 * @param[in] handle      CAN句柄
 * @param[in] filter_id   过滤ID（要匹配的CAN ID）
 * @param[in] filter_mask 过滤掩码（1表示该位必须匹配，0表示忽略）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 * @return OSAL_ERR_GENERIC       设置失败
 *
 * @note 过滤规则：(received_id & filter_mask) == (filter_id & filter_mask)
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_CAN_SetFilter(hal_can_handle_t handle, uint32_t filter_id, uint32_t filter_mask);

#ifdef __cplusplus
}
#endif

#endif /* HAL_CAN_H */
