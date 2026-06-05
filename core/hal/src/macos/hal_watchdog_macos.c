/************************************************************************
 * HAL层 - Watchdog驱动 (macOS 打桩实现)
 *
 * 说明：
 * - 这是 macOS 平台的打桩实现，仅用于编译
 * - 所有函数返回 OSAL_ERR_NOT_IMPLEMENTED
 * - 实际硬件访问需要在 Linux 平台上运行
 ************************************************************************/

#include "hal.h"
#include "osal.h"

int32_t HAL_WATCHDOG_Init(const hal_watchdog_config_t *config, hal_watchdog_handle_t *handle)
{
    (void)config;
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_Deinit(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_Kick(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_Enable(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_Disable(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_SetTimeout(hal_watchdog_handle_t handle, uint32_t timeout_sec)
{
    (void)handle;
    (void)timeout_sec;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_GetTimeout(hal_watchdog_handle_t handle, uint32_t *timeout_sec)
{
    (void)handle;
    (void)timeout_sec;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_GetTimeleft(hal_watchdog_handle_t handle, uint32_t *timeleft_sec)
{
    (void)handle;
    (void)timeleft_sec;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_GetStats(hal_watchdog_handle_t handle, uint32_t *kick_count)
{
    (void)handle;
    (void)kick_count;
    return OSAL_ERR_NOT_IMPLEMENTED;
}
