/************************************************************************
 * HAL层 - I2C驱动 (macOS 打桩实现)
 *
 * 说明：
 * - 这是 macOS 平台的打桩实现，仅用于编译
 * - 所有函数返回 OSAL_ERR_NOT_IMPLEMENTED
 * - 实际硬件访问需要在 Linux 平台上运行
 ************************************************************************/

#include "osal.h"
#include "hal.h"

int32_t HAL_I2C_open(const hal_i2c_config_t *config, hal_i2c_handle_t *handle)
{
    (void)config;
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_I2C_close(hal_i2c_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_I2C_write(hal_i2c_handle_t handle, uint16_t slave_addr,
                      const uint8_t *buffer, uint32_t size)
{
    (void)handle;
    (void)slave_addr;
    (void)buffer;
    (void)size;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_I2C_read(hal_i2c_handle_t handle, uint16_t slave_addr,
                     uint8_t *buffer, uint32_t size)
{
    (void)handle;
    (void)slave_addr;
    (void)buffer;
    (void)size;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_I2C_write_reg(hal_i2c_handle_t handle, uint16_t slave_addr,
                         uint8_t reg_addr, const uint8_t *buffer, uint32_t size)
{
    (void)handle;
    (void)slave_addr;
    (void)reg_addr;
    (void)buffer;
    (void)size;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_I2C_read_reg(hal_i2c_handle_t handle, uint16_t slave_addr,
                        uint8_t reg_addr, uint8_t *buffer, uint32_t size)
{
    (void)handle;
    (void)slave_addr;
    (void)reg_addr;
    (void)buffer;
    (void)size;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_I2C_transfer(hal_i2c_handle_t handle, i2c_msg_t *msgs, uint32_t num)
{
    (void)handle;
    (void)msgs;
    (void)num;
    return OSAL_ERR_NOT_IMPLEMENTED;
}
