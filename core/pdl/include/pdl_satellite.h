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
    STATUS_OK = 0x00,
    STATUS_ERROR = 0x01,
    STATUS_BUSY = 0x02,
    STATUS_TIMEOUT = 0x03
} can_status_t;

/*
 * 卫星平台服务句柄
 */
typedef void* satellite_service_handle_t;

/*
 * 卫星平台服务配置
 */
typedef struct
{
    const char *can_device;        /* CAN设备名 */
    uint32_t can_bitrate;            /* CAN波特率 */
    uint32_t heartbeat_interval_ms;  /* 心跳间隔(ms) */
    uint32_t cmd_timeout_ms;         /* 命令超时(ms) */
} satellite_service_config_t;

/*
 * 命令回调函数类型
 */
typedef void (*satellite_cmd_callback_t)(uint8_t cmd_type, uint32_t param, void *user_data);

/**
 * @brief 初始化卫星平台服务
 *
 * @param[in] config 配置参数
 * @param[out] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_Satellite_Init(const satellite_service_config_t *config,
                         satellite_service_handle_t *handle);

/**
 * @brief 反初始化卫星平台服务
 *
 * @param[in] handle 服务句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_Deinit(satellite_service_handle_t handle);

/**
 * @brief 注册命令回调函数
 *
 * @param[in] handle 服务句柄
 * @param[in] callback 回调函数
 * @param[in] user_data 用户数据
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_RegisterCallback(satellite_service_handle_t handle,
                                     satellite_cmd_callback_t callback,
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
int32_t PDL_Satellite_SendResponse(satellite_service_handle_t handle,
                                 uint32_t seq_num,
                                 can_status_t status,
                                 uint32_t result);

/**
 * @brief 发送心跳到卫星平台
 *
 * @param[in] handle 服务句柄
 * @param[in] status 当前状态
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t PDL_Satellite_SendHeartbeat(satellite_service_handle_t handle,
                                  can_status_t status);

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
int32_t PDL_Satellite_GetStats(satellite_service_handle_t handle,
                             uint32_t *rx_count,
                             uint32_t *tx_count,
                             uint32_t *error_count);

#endif /* PDL_SATELLITE_H */
