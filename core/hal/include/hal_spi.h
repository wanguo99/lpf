/************************************************************************
 * HAL层 - SPI驱动接口
 *
 * 提供统一的SPI总线访问接口
 ************************************************************************/

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "osal_types.h"
#include "config/spi_types.h"

/*===========================================================================
 * 锁配置
 *===========================================================================*/

/**
 * @brief SPI 驱动文件锁路径格式
 *
 * 参数：device - SPI 设备名（如 spidev0.0）
 * 示例：/var/lock/hal_spi_spidev0.0.lock
 *
 * 注意：需要包含 osal_flock.h 以使用 OSAL_LOCK_DIR
 */
#define HAL_SPI_LOCK_PATH_FMT    OSAL_LOCK_DIR "/hal_spi_%s.lock"

/**
 * @brief SPI 驱动文件锁超时时间（毫秒）
 */
#define HAL_SPI_LOCK_TIMEOUT_MS  OSAL_LOCK_DEFAULT_TIMEOUT_MS

/*===========================================================================
 * 类型定义
 *===========================================================================*/

typedef void* hal_spi_handle_t;

typedef struct
{
    const char *device;         /* e.g., "/dev/spidev0.0" */
    uint8_t     mode;           /* SPI模式 (SPI_MODE_0~3) */
    uint8_t     bits_per_word;  /* 每字位数 (通常为8) */
    uint32_t    max_speed_hz;   /* 最大速率 (Hz) */
    uint32_t    timeout;        /* 传输超时(ms) */
} hal_spi_config_t;

/**
 * @brief 打开SPI设备
 *
 * @param[in] config SPI配置
 * @param[out] handle 返回的SPI句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_SPI_Open(const hal_spi_config_t *config, hal_spi_handle_t *handle);

/**
 * @brief 关闭SPI设备
 *
 * @param[in] handle SPI句柄
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t HAL_SPI_Close(hal_spi_handle_t handle);

/**
 * @brief SPI写操作
 *
 * @param[in] handle SPI句柄
 * @param[in] buffer 发送缓冲区
 * @param[in] size 数据长度
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_SPI_Write(hal_spi_handle_t handle, const uint8_t *buffer, uint32_t size);

/**
 * @brief SPI读操作
 *
 * @param[in] handle SPI句柄
 * @param[out] buffer 接收缓冲区
 * @param[in] size 数据长度
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_SPI_Read(hal_spi_handle_t handle, uint8_t *buffer, uint32_t size);

/**
 * @brief SPI全双工传输
 *
 * @param[in] handle SPI句柄
 * @param[in] tx_buffer 发送缓冲区
 * @param[out] rx_buffer 接收缓冲区
 * @param[in] size 数据长度
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_SPI_Transfer(hal_spi_handle_t handle, const uint8_t *tx_buffer,
                         uint8_t *rx_buffer, uint32_t size);

/**
 * @brief SPI批量传输 (支持多段传输)
 *
 * @param[in] handle SPI句柄
 * @param[in] transfers 传输数组
 * @param[in] num 传输数量
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_SPI_TransferMulti(hal_spi_handle_t handle, hal_spi_transfer_t *transfers, uint32_t num);

/**
 * @brief 设置SPI配置
 *
 * @param[in] handle SPI句柄
 * @param[in] config SPI配置
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t HAL_SPI_SetConfig(hal_spi_handle_t handle, const hal_spi_config_t *config);

#endif /* HAL_SPI_H */
