/************************************************************************
 * HAL层 - 看门狗驱动内部定义
 *
 * 本文件包含看门狗驱动的内部实现细节，不对外暴露
 ************************************************************************/

#ifndef HAL_WATCHDOG_INTERNAL_H
#define HAL_WATCHDOG_INTERNAL_H

#include <stdatomic.h>
#include "osal.h"
#include "hal.h"
#include "hal_watchdog.h"

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

/**
 * @brief 看门狗设备上下文（内部实现）
 *
 * 看门狗通常是单进程管理，不需要文件锁保护
 */
typedef struct
{
	int32_t fd;                       /* 看门狗设备文件描述符 */
	char    device[256];              /* 设备路径 */
	uint32_t timeout_sec;             /* 超时时间（秒） */
	bool    enabled;                  /* 启用状态 */
	_Atomic uint32_t kick_count;      /* 喂狗次数（原子操作） */
} hal_watchdog_context_t;

/*===========================================================================
 * 平台特定常量
 *===========================================================================*/

/**
 * @brief 默认看门狗超时时间（秒）
 */
#define HAL_WATCHDOG_DEFAULT_TIMEOUT_SEC  0x3C

/**
 * @brief 看门狗魔术字符（Linux特有，用于安全关闭）
 */
#define HAL_WATCHDOG_MAGIC_CLOSE_CHAR     'V'

#endif /* HAL_WATCHDOG_INTERNAL_H */
