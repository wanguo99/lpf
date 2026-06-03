/************************************************************************
 * HAL层 - 串口驱动 (macOS 打桩实现)
 *
 * 说明：
 * - 这是 macOS 平台的打桩实现，仅用于编译
 * - 所有函数返回 OSAL_ERR_NOT_IMPLEMENTED
 * - 实际硬件访问需要在 Linux 平台上运行
 ************************************************************************/

#include "hal/hal_serial_api.h"
#include "osal.h"

int32_t HAL_Serial_Open(const char *device, const hal_serial_config_t *config,
                      hal_serial_handle_t *handle)
{
    (void)device;
    (void)config;
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_Serial_Close(hal_serial_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_Serial_Write(hal_serial_handle_t handle, const void *buffer,
                       uint32_t size, int32_t timeout)
{
    (void)handle;
    (void)buffer;
    (void)size;
    (void)timeout;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_Serial_Read(hal_serial_handle_t handle, void *buffer,
                      uint32_t size, int32_t timeout)
{
    (void)handle;
    (void)buffer;
    (void)size;
    (void)timeout;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_Serial_Flush(hal_serial_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_Serial_SetConfig(hal_serial_handle_t handle,
                           const hal_serial_config_t *config)
{
    (void)handle;
    (void)config;
    return OSAL_ERR_NOT_IMPLEMENTED;
}
