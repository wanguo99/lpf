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

int32_t HAL_WATCHDOG_init(const hal_watchdog_config_t *config, hal_watchdog_handle_t *handle)
{
    (void)config;
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_deinit(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_kick(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_enable(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_disable(hal_watchdog_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_set_timeout(hal_watchdog_handle_t handle, uint32_t timeout_sec)
{
    (void)handle;
    (void)timeout_sec;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_get_timeout(hal_watchdog_handle_t handle, uint32_t *timeout_sec)
{
    (void)handle;
    (void)timeout_sec;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_get_timeleft(hal_watchdog_handle_t handle, uint32_t *timeleft_sec)
{
    (void)handle;
    (void)timeleft_sec;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_WATCHDOG_get_stats(hal_watchdog_handle_t handle, uint32_t *kick_count)
{
    (void)handle;
    (void)kick_count;
    return OSAL_ERR_NOT_IMPLEMENTED;
}
