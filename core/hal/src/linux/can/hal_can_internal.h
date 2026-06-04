/************************************************************************
 * HAL层 - CAN驱动内部定义
 *
 * 本文件包含CAN驱动的内部实现细节，不对外暴露
 ************************************************************************/

#ifndef HAL_CAN_INTERNAL_H
#define HAL_CAN_INTERNAL_H

#include "osal/osal.h"
#include "osal/osal.h"
#include "hal/hal_can_api.h"

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

/*===========================================================================
 * 锁配置
 *===========================================================================*/

/**
 * @brief CAN 驱动文件锁路径格式
 *
 * 参数：interface - CAN 接口名（如 can0, can1）
 * 示例：/var/lock/hal_can_can0.lock
 */
#define HAL_CAN_LOCK_PATH_FMT    OSAL_LOCK_DIR "/hal_can_%s.lock"

/**
 * @brief CAN 驱动文件锁超时时间（毫秒）
 */
#define HAL_CAN_LOCK_TIMEOUT_MS  OSAL_LOCK_DEFAULT_TIMEOUT_MS

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

/**
 * @brief CAN 设备上下文（内部实现）
 *
 * 采用双重保护机制：
 * - flock: 文件锁，保护进程间并发访问
 * - mutex: 互斥锁，保护线程间并发访问
 */
typedef struct
{
	int32_t sockfd;                /* SocketCAN 套接字文件描述符 */
	char    interface[IFNAMSIZ];   /* CAN接口名 */
	uint32_t baudrate;             /* 波特率 */
	bool    initialized;           /* 初始化标志 */

	/* 双重保护机制 */
	osal_flock_t *flock;           /* 文件锁（进程间保护） */
	osal_mutex_t *mutex;           /* 互斥锁（线程间保护） */
} hal_can_context_t;

#endif /* HAL_CAN_INTERNAL_H */
