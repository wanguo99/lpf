/************************************************************************
 * HAL层 - Watchdog驱动接口
 *
 * 提供统一的看门狗访问接口
 ************************************************************************/

#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

#include "osal/osal_types.h"

typedef void* hal_watchdog_handle_t;

typedef struct
{
    const char *device;         /* 设备路径，如 "/dev/watchdog" */
    uint32_t timeout_sec;       /* 超时时间（秒），0表示使用默认值 */
    bool enable_on_init;        /* 初始化时是否立即启用看门狗 */
} hal_watchdog_config_t;

/**
 * @brief 初始化Watchdog驱动
 *
 * @param[in] config Watchdog配置
 * @param[out] handle 返回的Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_WATCHDOG_Init(const hal_watchdog_config_t *config, hal_watchdog_handle_t *handle);

/**
 * @brief 关闭Watchdog驱动
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 */
int32_t HAL_WATCHDOG_Deinit(hal_watchdog_handle_t handle);

/**
 * @brief 喂狗（重置看门狗定时器）
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_WATCHDOG_Kick(hal_watchdog_handle_t handle);

/**
 * @brief 启用看门狗
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_WATCHDOG_Enable(hal_watchdog_handle_t handle);

/**
 * @brief 禁用看门狗
 *
 * 注意：某些硬件看门狗一旦启用就无法禁用
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_NOT_SUPPORTED 硬件不支持禁用
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_WATCHDOG_Disable(hal_watchdog_handle_t handle);

/**
 * @brief 设置看门狗超时时间
 *
 * @param[in] handle Watchdog句柄
 * @param[in] timeout_sec 超时时间（秒）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_WATCHDOG_SetTimeout(hal_watchdog_handle_t handle, uint32_t timeout_sec);

/**
 * @brief 获取看门狗超时时间
 *
 * @param[in] handle Watchdog句柄
 * @param[out] timeout_sec 返回的超时时间（秒）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_WATCHDOG_GetTimeout(hal_watchdog_handle_t handle, uint32_t *timeout_sec);

/**
 * @brief 获取看门狗剩余时间
 *
 * @param[in] handle Watchdog句柄
 * @param[out] timeleft_sec 返回的剩余时间（秒）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_NOT_SUPPORTED 硬件不支持此功能
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_WATCHDOG_GetTimeleft(hal_watchdog_handle_t handle, uint32_t *timeleft_sec);

/**
 * @brief 获取看门狗统计信息
 *
 * @param[in] handle Watchdog句柄
 * @param[out] kick_count 喂狗次数
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 */
int32_t HAL_WATCHDOG_GetStats(hal_watchdog_handle_t handle, uint32_t *kick_count);

#endif /* HAL_WATCHDOG_H */
