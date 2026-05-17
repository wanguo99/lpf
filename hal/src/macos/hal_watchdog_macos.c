/************************************************************************
 * HAL层 - Watchdog驱动macOS桩实现
 *
 * 仅用于编译，不提供实际功能
 ************************************************************************/

#include "hal_watchdog.h"
#include "osal.h"

/**
 * @brief 初始化Watchdog驱动（桩实现）
 */
int32_t HAL_WATCHDOG_Init(const hal_watchdog_config_t *config, hal_watchdog_handle_t *handle)
{
    (void)config;
    (void)handle;
    LOG_WARN("HAL_WDT", "Watchdog not supported on macOS (stub implementation)");
    return OSAL_ERR_NOT_SUPPORTED;
}

/**
 * @brief 关闭Watchdog驱动（桩实现）
 */
int32_t HAL_WATCHDOG_Deinit(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_SUPPORTED;
}

/**
 * @brief 喂狗（桩实现）
 */
int32_t HAL_WATCHDOG_Kick(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_SUPPORTED;
}

/**
 * @brief 启用看门狗（桩实现）
 */
int32_t HAL_WATCHDOG_Enable(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_SUPPORTED;
}

/**
 * @brief 禁用看门狗（桩实现）
 */
int32_t HAL_WATCHDOG_Disable(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_SUPPORTED;
}

/**
 * @brief 设置看门狗超时时间（桩实现）
 */
int32_t HAL_WATCHDOG_SetTimeout(hal_watchdog_handle_t handle, uint32_t timeout_sec)
{
    (void)handle;
    (void)timeout_sec;
    return OSAL_ERR_NOT_SUPPORTED;
}

/**
 * @brief 获取看门狗超时时间（桩实现）
 */
int32_t HAL_WATCHDOG_GetTimeout(hal_watchdog_handle_t handle, uint32_t *timeout_sec)
{
    (void)handle;
    (void)timeout_sec;
    return OSAL_ERR_NOT_SUPPORTED;
}

/**
 * @brief 获取看门狗剩余时间（桩实现）
 */
int32_t HAL_WATCHDOG_GetTimeleft(hal_watchdog_handle_t handle, uint32_t *timeleft_sec)
{
    (void)handle;
    (void)timeleft_sec;
    return OSAL_ERR_NOT_SUPPORTED;
}

/**
 * @brief 获取看门狗统计信息（桩实现）
 */
int32_t HAL_WATCHDOG_GetStats(hal_watchdog_handle_t handle, uint32_t *kick_count)
{
    (void)handle;
    if (kick_count != NULL)
    {
        *kick_count = 0;
    }
    return OSAL_ERR_NOT_SUPPORTED;
}
