/************************************************************************
 * HAL层 - CAN驱动 (macOS 打桩实现)
 *
 * 说明：
 * - 这是 macOS 平台的打桩实现，仅用于编译
 * - 所有函数返回 OSAL_ERR_NOT_IMPLEMENTED
 * - 实际硬件访问需要在 Linux 平台上运行
 ************************************************************************/

#include "hal_can.h"
#include "osal.h"

int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle)
{
    (void)config;
    (void)handle;

    LOG_WARN("HAL_CAN", "CAN driver not implemented on macOS (stub only)");
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_CAN_Deinit(hal_can_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_CAN_Send(hal_can_handle_t handle, const can_frame_t *frame)
{
    (void)handle;
    (void)frame;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_CAN_Recv(hal_can_handle_t handle, can_frame_t *frame, int32_t timeout)
{
    (void)handle;
    (void)frame;
    (void)timeout;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_CAN_SetFilter(hal_can_handle_t handle, uint32_t filter_id, uint32_t filter_mask)
{
    (void)handle;
    (void)filter_id;
    (void)filter_mask;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_CAN_GetStats(hal_can_handle_t handle,
                       uint32_t *tx_count,
                       uint32_t *rx_count,
                       uint32_t *err_count)
{
    if (NULL == handle)
        return OSAL_ERR_INVALID_ID;

    if (tx_count != NULL)
        *tx_count = 0;
    if (rx_count != NULL)
        *rx_count = 0;
    if (err_count != NULL)
        *err_count = 0;

    return OSAL_SUCCESS;
}
