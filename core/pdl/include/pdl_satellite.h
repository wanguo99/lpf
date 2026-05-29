/************************************************************************
 * 卫星平台接口服务
 *
 * 功能：
 * - 封装CAN总线通信协议
 * - 处理平台命令请求和响应
 * - 管理心跳和状态上报
 * - 提供统一的平台交互接口
 ************************************************************************/

#ifndef PDL_SATELLITE_H
#define PDL_SATELLITE_H

#include "osal_types.h"

/* CAN状态码 */
typedef enum
{
    PDL_SATELLITE_STATUS_OK = 0x00,
    PDL_SATELLITE_STATUS_ERROR = 0x01,
    PDL_SATELLITE_STATUS_BUSY = 0x02,
    PDL_SATELLITE_STATUS_TIMEOUT = 0x03
} pdl_satellite_status_t;

/*
 * 卫星平台服务句柄
 */
typedef void* pdl_satellite_handle_t;

/*
 * 卫星平台服务配置
 *
 * 说明：直接嵌入 HAL 层配置结构体，避免重复定义
 */
typedef struct
{
    /* CAN配置 - 嵌入 HAL 配置 */
    const char *can_device;        /* CAN设备名（传递给 HAL） */
    uint32_t can_bitrate;            /* CAN波特率（传递给 HAL） */
    uint32_t can_rx_timeout;         /* CAN接收超时（传递给 HAL） */
    uint32_t can_tx_timeout;         /* CAN发送超时（传递给 HAL） */

    /* 业务配置 */
    uint32_t heartbeat_interval_ms;  /* 心跳间隔(ms) */
    uint32_t cmd_timeout_ms;         /* 命令超时(ms) */
} pdl_satellite_config_t;

/*
 * 命令回调函数类型
 */
typedef void (*pdl_satellite_cmd_callback_t)(uint8_t cmd_type, uint32_t param, void *user_data);

/**
 * @brief 初始化卫星平台服务
 *
 * @param[in] config 配置参数
 * @param[out] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_Satellite_Init(const pdl_satellite_config_t *config,
                         pdl_satellite_handle_t *handle);

/**
 * @brief 反初始化卫星平台服务
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_Deinit(pdl_satellite_handle_t handle);

/**
 * @brief 注册命令回调函数
 *
 * @param[in] handle 服务句柄
 * @param[in] callback 回调函数
 * @param[in] user_data 用户数据
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_RegisterCallback(pdl_satellite_handle_t handle,
                                     pdl_satellite_cmd_callback_t callback,
                                     void *user_data);

/**
 * @brief 发送响应到卫星平台
 *
 * @param[in] handle 服务句柄
 * @param[in] seq_num 序列号
 * @param[in] status 状态码
 * @param[in] result 结果数据
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_SendResponse(pdl_satellite_handle_t handle,
                                 uint32_t seq_num,
                                 pdl_satellite_status_t status,
                                 uint32_t result);

/**
 * @brief 发送心跳到卫星平台
 *
 * @param[in] handle 服务句柄
 * @param[in] status 当前状态
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_SendHeartbeat(pdl_satellite_handle_t handle,
                                  pdl_satellite_status_t status);

/**
 * @brief 获取服务统计信息
 *
 * @param[in] handle 服务句柄
 * @param[out] rx_count 接收计数
 * @param[out] tx_count 发送计数
 * @param[out] error_count 错误计数
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_GetStats(pdl_satellite_handle_t handle,
                             uint32_t *rx_count,
                             uint32_t *tx_count,
                             uint32_t *error_count);

#endif /* PDL_SATELLITE_H */
