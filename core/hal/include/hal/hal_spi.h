/************************************************************************
 * HAL层 - SPI总线硬件抽象层API
 *
 * 提供统一的SPI总线访问接口（基于Linux /dev/spidev*）
 ************************************************************************/

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "config/hal_spi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 类型定义
 *===========================================================================*/

/**
 * @brief SPI设备句柄（不透明指针）
 */
typedef void* hal_spi_handle_t;

/**
 * @brief SPI配置结构
 */
typedef struct
{
	const char *device;        /* SPI设备路径，如 "/dev/spidev0.0" */
	uint8_t     mode;          /* SPI模式 (0-3)，参见spi_types.h */
	uint8_t     bits_per_word; /* 每字位数（通常为8） */
	uint32_t    max_speed_hz;  /* 最大传输速率（Hz） */
	uint32_t    timeout;       /* 传输超时时间（ms） */
} hal_spi_config_t;

/*===========================================================================
 * API函数
 *===========================================================================*/

/**
 * @brief 打开SPI设备
 *
 * 打开指定的SPI设备并配置参数。
 *
 * @param[in]  config  SPI配置参数
 * @param[out] handle  返回的SPI句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NO_DEVICE     设备不存在
 * @return OSAL_ERR_GENERIC       打开失败
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SPI_open(const hal_spi_config_t *config, hal_spi_handle_t *handle);

/**
 * @brief 关闭SPI设备
 *
 * 释放SPI资源，关闭设备文件。
 *
 * @param[in] handle SPI句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SPI_close(hal_spi_handle_t handle);

/**
 * @brief SPI写操作
 *
 * 向SPI设备发送数据（忽略接收数据）。
 *
 * @param[in] handle SPI句柄
 * @param[in] buffer 发送缓冲区
 * @param[in] size   数据长度（字节）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       写入失败
 *
 * @note 使用配置的timeout作为超时时间
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SPI_write(hal_spi_handle_t handle, const uint8_t *buffer, uint32_t size);

/**
 * @brief SPI读操作
 *
 * 从SPI设备接收数据（发送全0或全1）。
 *
 * @param[in]  handle SPI句柄
 * @param[out] buffer 接收缓冲区
 * @param[in]  size   数据长度（字节）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       读取失败
 *
 * @note 使用配置的timeout作为超时时间
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SPI_read(hal_spi_handle_t handle, uint8_t *buffer, uint32_t size);

/**
 * @brief SPI全双工传输
 *
 * 同时发送和接收数据（全双工模式）。
 *
 * @param[in]  handle    SPI句柄
 * @param[in]  tx_buffer 发送缓冲区
 * @param[out] rx_buffer 接收缓冲区
 * @param[in]  size      数据长度（字节）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       传输失败
 *
 * @note tx_buffer和rx_buffer可以指向同一缓冲区（原地操作）
 * @note 使用配置的timeout作为超时时间
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SPI_transfer(hal_spi_handle_t handle, const uint8_t *tx_buffer,
                         uint8_t *rx_buffer, uint32_t size);

/**
 * @brief SPI批量传输（支持多段传输）
 *
 * 执行一个或多个SPI传输段。可用于实现复杂的SPI通信协议。
 *
 * @param[in] handle    SPI句柄
 * @param[in] transfers 传输段数组
 * @param[in] num       传输段数量
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       传输失败
 *
 * @note 使用Linux SPI_IOC_MESSAGE(n) ioctl实现批量传输
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SPI_transfer_multi(hal_spi_handle_t handle, hal_spi_transfer_t *transfers, uint32_t num);

/**
 * @brief 设置SPI配置
 *
 * 动态修改SPI参数（模式、速率、位宽等）。
 *
 * @param[in] handle SPI句柄
 * @param[in] config 新的SPI配置
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       配置失败
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t HAL_SPI_set_config(hal_spi_handle_t handle, const hal_spi_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* HAL_SPI_H */
