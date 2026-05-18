/************************************************************************
 * HAL层 - I2C驱动接口
 *
 * 提供统一的I2C总线访问接口
 ************************************************************************/

#ifndef HAL_I2C_H
#define HAL_I2C_H

#include "osal_types.h"
#include "config/i2c_types.h"

typedef void* hal_i2c_handle_t;

typedef struct
{
    const char *device;     /* e.g., "/dev/i2c-0" */
    uint32_t    timeout;    /* 传输超时(ms) */
} hal_i2c_config_t;

/**
 * @brief 打开I2C设备
 *
 * @param[in] config I2C配置
 * @param[out] handle 返回的I2C句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_I2C_Open(const hal_i2c_config_t *config, hal_i2c_handle_t *handle);

/**
 * @brief 关闭I2C设备
 *
 * @param[in] handle I2C句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t HAL_I2C_Close(hal_i2c_handle_t handle);

/**
 * @brief 写入数据到I2C从设备
 *
 * @param[in] handle I2C句柄
 * @param[in] slave_addr 从设备地址 (7位)
 * @param[in] buffer 数据缓冲区
 * @param[in] size 数据长度
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 其他错误
 */
int32_t HAL_I2C_Write(hal_i2c_handle_t handle, uint16_t slave_addr,
                      const uint8_t *buffer, uint32_t size);

/**
 * @brief 从I2C从设备读取数据
 *
 * @param[in] handle I2C句柄
 * @param[in] slave_addr 从设备地址 (7位)
 * @param[out] buffer 数据缓冲区
 * @param[in] size 数据长度
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 其他错误
 */
int32_t HAL_I2C_Read(hal_i2c_handle_t handle, uint16_t slave_addr,
                     uint8_t *buffer, uint32_t size);

/**
 * @brief 写寄存器操作 (先写寄存器地址，再写数据)
 *
 * @param[in] handle I2C句柄
 * @param[in] slave_addr 从设备地址 (7位)
 * @param[in] reg_addr 寄存器地址
 * @param[in] buffer 数据缓冲区
 * @param[in] size 数据长度
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_I2C_WriteReg(hal_i2c_handle_t handle, uint16_t slave_addr,
                         uint8_t reg_addr, const uint8_t *buffer, uint32_t size);

/**
 * @brief 读寄存器操作 (先写寄存器地址，再读数据)
 *
 * @param[in] handle I2C句柄
 * @param[in] slave_addr 从设备地址 (7位)
 * @param[in] reg_addr 寄存器地址
 * @param[out] buffer 数据缓冲区
 * @param[in] size 数据长度
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_I2C_ReadReg(hal_i2c_handle_t handle, uint16_t slave_addr,
                        uint8_t reg_addr, uint8_t *buffer, uint32_t size);

/**
 * @brief 执行I2C传输 (支持组合传输)
 *
 * @param[in] handle I2C句柄
 * @param[in] msgs 传输消息数组
 * @param[in] num 消息数量
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_I2C_Transfer(hal_i2c_handle_t handle, i2c_msg_t *msgs, uint32_t num);

#endif /* HAL_I2C_H */
