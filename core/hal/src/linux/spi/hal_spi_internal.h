/************************************************************************
 * HAL层 - SPI驱动内部定义
 *
 * 本文件包含SPI驱动的内部实现细节，不对外暴露
 ************************************************************************/

#ifndef HAL_SPI_INTERNAL_H
#define HAL_SPI_INTERNAL_H

#include "osal/osal.h"
#include "hal/hal.h"

/*===========================================================================
 * 锁配置
 *===========================================================================*/

/**
 * @brief SPI 驱动文件锁路径格式
 *
 * 参数：device - SPI 设备名（如 spidev0.0）
 * 示例：/var/lock/hal_spi_spidev0.0.lock
 */
#define HAL_SPI_LOCK_PATH_FMT    OSAL_LOCK_DIR "/hal_spi_%s.lock"

/**
 * @brief SPI 驱动文件锁超时时间（毫秒）
 */
#define HAL_SPI_LOCK_TIMEOUT_MS  OSAL_LOCK_DEFAULT_TIMEOUT_MS

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

/**
 * @brief SPI 设备上下文（内部实现）
 *
 * 采用双重保护机制：
 * - flock: 文件锁，保护进程间并发访问
 * - mutex: 互斥锁，保护线程间并发访问
 */
typedef struct
{
	int32_t  fd;                 /* SPI设备文件描述符 */
	char     device[256];        /* 设备路径 */
	uint8_t  mode;               /* SPI模式 */
	uint8_t  bits_per_word;      /* 每字位数 */
	uint32_t max_speed_hz;       /* 最大传输速率 */
	uint32_t timeout;            /* 传输超时时间（ms） */
	bool     initialized;        /* 初始化标志 */

	/* 双重保护机制 */
	osal_flock_t *flock;         /* 文件锁（进程间保护） */
	osal_mutex_t *mutex;         /* 互斥锁（线程间保护） */
} hal_spi_context_t;

#endif /* HAL_SPI_INTERNAL_H */
