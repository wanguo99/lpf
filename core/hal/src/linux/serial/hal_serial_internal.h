/************************************************************************
 * HAL层 - 串口驱动内部定义
 *
 * 本文件包含串口驱动的内部实现细节，不对外暴露
 ************************************************************************/

#ifndef HAL_SERIAL_INTERNAL_H
#define HAL_SERIAL_INTERNAL_H

#include "osal/osal.h"
#include "hal/hal.h"

/*===========================================================================
 * 锁配置
 *===========================================================================*/

/**
 * @brief Serial 驱动文件锁路径格式
 *
 * 参数：device - 串口设备名（如 ttyS0, ttyUSB0）
 * 示例：/var/lock/hal_serial_ttyS0.lock
 */
#define HAL_SERIAL_LOCK_PATH_FMT    OSAL_LOCK_DIR "/hal_serial_%s.lock"

/**
 * @brief Serial 驱动文件锁超时时间（毫秒）
 */
#define HAL_SERIAL_LOCK_TIMEOUT_MS  OSAL_LOCK_DEFAULT_TIMEOUT_MS

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

/**
 * @brief 串口设备上下文（内部实现）
 *
 * 采用双重保护机制：
 * - flock: 文件锁，保护进程间并发访问
 * - mutex: 互斥锁，保护线程间并发访问
 */
typedef struct
{
	int32_t fd;                     /* 串口文件描述符 */
	char    device[256];            /* 设备路径 */
	hal_serial_config_t config;     /* 当前配置 */
	bool    initialized;            /* 初始化标志 */

	/* 双重保护机制 */
	osal_flock_t *flock;            /* 文件锁（进程间保护） */
	osal_mutex_t *mutex;            /* 互斥锁（线程间保护） */
} hal_serial_context_t;

/*===========================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 波特率转换表（POSIX termios速度常量）
 *
 * 将数值波特率转换为termios的Bxxxx常量
 */
typedef struct {
	uint32_t baud_rate;
	uint32_t speed_const;
} baud_rate_map_t;

#endif /* HAL_SERIAL_INTERNAL_H */
