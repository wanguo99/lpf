/************************************************************************
 * HAL层 - CAN驱动接口
 *
 * 提供统一的CAN总线访问接口
 ************************************************************************/

#ifndef HAL_CAN_H
#define HAL_CAN_H

#include "osal_types.h"
#include "config/can_types.h"

/*===========================================================================
 * 锁配置
 *===========================================================================*/

/**
 * @brief CAN 驱动文件锁路径格式
 *
 * 参数：interface - CAN 接口名（如 can0, can1）
 * 示例：/var/lock/hal_can_can0.lock
 *
 * 注意：需要包含 osal_flock.h 以使用 OSAL_LOCK_DIR
 */
#define HAL_CAN_LOCK_PATH_FMT    OSAL_LOCK_DIR "/hal_can_%s.lock"

/**
 * @brief CAN 驱动文件锁超时时间（毫秒）
 */
#define HAL_CAN_LOCK_TIMEOUT_MS  OSAL_LOCK_DEFAULT_TIMEOUT_MS

/*===========================================================================
 * 类型定义
 *===========================================================================*/

typedef void* hal_can_handle_t;

typedef struct
{
    const char *interface;  /* e.g., "can0", "vcan0" */
    uint32_t      baudrate;
    uint32_t      rx_timeout;
    uint32_t      tx_timeout;
} hal_can_config_t;

/**
 * @brief 初始化CAN驱动
 *
 * @param[in] config CAN配置
 * @param[out] handle 返回的CAN句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle);

/**
 * @brief 关闭CAN驱动
 *
 * @param[in] handle CAN句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t HAL_CAN_Deinit(hal_can_handle_t handle);

/**
 * @brief 发送CAN帧
 *
 * @param[in] handle CAN句柄
 * @param[in] frame CAN帧
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 其他错误
 */
int32_t HAL_CAN_Send(hal_can_handle_t handle, const hal_can_frame_t *frame);

/**
 * @brief 接收CAN帧
 *
 * @param[in] handle CAN句柄
 * @param[out] frame 接收到的CAN帧
 * @param[in] timeout 超时时间(ms), OS_PEND表示永久等待
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 其他错误
 */
int32_t HAL_CAN_Recv(hal_can_handle_t handle, hal_can_frame_t *frame, int32_t timeout);

/**
 * @brief 设置CAN过滤器
 *
 * @param[in] handle CAN句柄
 * @param[in] filter_id 过滤ID
 * @param[in] filter_mask 过滤掩码
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_CAN_SetFilter(hal_can_handle_t handle, uint32_t filter_id, uint32_t filter_mask);

#endif /* HAL_CAN_H */
