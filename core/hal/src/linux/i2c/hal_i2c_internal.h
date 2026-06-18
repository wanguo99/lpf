/************************************************************************
 * HAL层 - I2C驱动内部定义
 *
 * 本文件包含I2C驱动的内部实现细节，不对外暴露
 ************************************************************************/

#ifndef HAL_I2C_INTERNAL_H
#define HAL_I2C_INTERNAL_H

#include "osal.h"
#include "hal.h"
#include "hal_i2c.h"
#include "../hal_linux_lock.h"

/*===========================================================================
 * 锁配置
 *===========================================================================*/

/**
 * @brief I2C 驱动文件锁路径格式
 *
 * 参数：bus - I2C 总线号（如 0, 1, 2）
 * 示例：/var/lock/hal_i2c_0.lock
 */
#define HAL_I2C_LOCK_PATH_FMT OSAL_LOCK_DIR "/hal_i2c_%d.lock"

/**
 * @brief I2C 驱动文件锁超时时间（毫秒）
 */
#define HAL_I2C_LOCK_TIMEOUT_MS OSAL_LOCK_DEFAULT_TIMEOUT_MS

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

/**
 * @brief I2C 设备上下文（内部实现）
 *
 * 通过 hal_linux_lock_t 统一封装进程间文件锁和线程互斥锁。
 */
typedef struct {
    int32_t fd; /* I2C设备文件描述符 */
    char device[256]; /* 设备路径 */
    uint32_t timeout; /* 传输超时时间（ms） */
    bool initialized; /* 初始化标志 */

    hal_linux_lock_t lock; /* 进程/线程保护 */
} hal_i2c_context_t;

#endif /* HAL_I2C_INTERNAL_H */
