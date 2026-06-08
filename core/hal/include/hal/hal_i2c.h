/************************************************************************
 * HAL层 - I2C总线硬件抽象层API
 *
 * 提供统一的I2C总线访问接口（基于Linux /dev/i2c-*）
 ************************************************************************/

#ifndef HAL_I2C_H
#define HAL_I2C_H

#include "config/hal_i2c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 类型定义
 *===========================================================================*/

/**
 * @brief I2C设备句柄（不透明指针）
 */
typedef void* hal_i2c_handle_t;

/**
 * @brief I2C配置结构
 */
typedef struct
{
	const char *device;  /* I2C设备路径，如 "/dev/i2c-0" */
	uint32_t    timeout; /* 传输超时时间（ms） */
} hal_i2c_config_t;

/*===========================================================================
 * API函数
 *===========================================================================*/

/**
 * @brief 打开I2C设备
 *
 * 打开指定的I2C总线设备文件。
 *
 * @param[in]  config  I2C配置参数
 * @param[out] handle  返回的I2C句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NO_DEVICE     设备不存在
 * @return OSAL_ERR_GENERIC       打开失败
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_I2C_Open(const hal_i2c_config_t *config, hal_i2c_handle_t *handle);

/**
 * @brief 关闭I2C设备
 *
 * 释放I2C资源，关闭设备文件。
 *
 * @param[in] handle I2C句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_I2C_Close(hal_i2c_handle_t handle);

/**
 * @brief 写入数据到I2C从设备
 *
 * 向指定的I2C从设备地址写入数据。
 *
 * @param[in] handle     I2C句柄
 * @param[in] slave_addr I2C从设备地址（7位地址）
 * @param[in] buffer     数据缓冲区
 * @param[in] size       数据长度（字节）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_TIMEOUT       传输超时
 * @return OSAL_ERR_GENERIC       写入失败（如NACK）
 *
 * @note 使用配置的timeout作为超时时间
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_I2C_Write(hal_i2c_handle_t handle, uint16_t slave_addr,
                      const uint8_t *buffer, uint32_t size);

/**
 * @brief 从I2C从设备读取数据
 *
 * 从指定的I2C从设备地址读取数据。
 *
 * @param[in]  handle     I2C句柄
 * @param[in]  slave_addr I2C从设备地址（7位地址）
 * @param[out] buffer     数据缓冲区
 * @param[in]  size       要读取的数据长度（字节）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_TIMEOUT       传输超时
 * @return OSAL_ERR_GENERIC       读取失败（如NACK）
 *
 * @note 使用配置的timeout作为超时时间
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_I2C_Read(hal_i2c_handle_t handle, uint16_t slave_addr,
                     uint8_t *buffer, uint32_t size);

/**
 * @brief 写寄存器操作（先写寄存器地址，再写数据）
 *
 * 常见的I2C寄存器写操作：发送寄存器地址后紧接着发送数据。
 *
 * @param[in] handle     I2C句柄
 * @param[in] slave_addr I2C从设备地址（7位地址）
 * @param[in] reg_addr   寄存器地址（8位）
 * @param[in] buffer     数据缓冲区
 * @param[in] size       数据长度（字节）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       写入失败
 *
 * @note 内部实现为组合传输：[START][ADDR+W][REG][DATA...][STOP]
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_I2C_WriteReg(hal_i2c_handle_t handle, uint16_t slave_addr,
                         uint8_t reg_addr, const uint8_t *buffer, uint32_t size);

/**
 * @brief 读寄存器操作（先写寄存器地址，再读数据）
 *
 * 常见的I2C寄存器读操作：发送寄存器地址后读取数据。
 *
 * @param[in]  handle     I2C句柄
 * @param[in]  slave_addr I2C从设备地址（7位地址）
 * @param[in]  reg_addr   寄存器地址（8位）
 * @param[out] buffer     数据缓冲区
 * @param[in]  size       要读取的数据长度（字节）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       读取失败
 *
 * @note 内部实现为组合传输：[START][ADDR+W][REG][RESTART][ADDR+R][DATA...][STOP]
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_I2C_ReadReg(hal_i2c_handle_t handle, uint16_t slave_addr,
                        uint8_t reg_addr, uint8_t *buffer, uint32_t size);

/**
 * @brief 执行I2C传输（支持组合传输）
 *
 * 执行一个或多个I2C消息的组合传输。支持复杂的I2C通信模式。
 *
 * @param[in] handle I2C句柄
 * @param[in] msgs   I2C消息数组
 * @param[in] num    消息数量
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       传输失败
 *
 * @note 使用Linux I2C_RDWR ioctl实现原子组合传输
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_I2C_Transfer(hal_i2c_handle_t handle, hal_i2c_msg_t *msgs, uint32_t num);

#ifdef __cplusplus
}
#endif

#endif /* HAL_I2C_H */
