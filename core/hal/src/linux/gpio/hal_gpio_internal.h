/************************************************************************
 * HAL层 - GPIO驱动内部定义
 *
 * 本文件包含GPIO驱动的内部实现细节，不对外暴露
 ************************************************************************/

#ifndef HAL_GPIO_INTERNAL_H
#define HAL_GPIO_INTERNAL_H

#include "osal.h"
#include "hal.h"
#include "hal_gpio.h"

/*===========================================================================
 * 锁配置
 *===========================================================================*/

/**
 * @brief GPIO 驱动文件锁路径（全局锁）
 *
 * GPIO 使用全局锁，所有 GPIO 操作共享同一个文件锁
 * 示例：/var/lock/hal_gpio.lock
 */
#define HAL_GPIO_LOCK_PATH       OSAL_LOCK_DIR "/hal_gpio.lock"

/**
 * @brief GPIO 驱动文件锁超时时间（毫秒）
 */
#define HAL_GPIO_LOCK_TIMEOUT_MS OSAL_LOCK_DEFAULT_TIMEOUT_MS

/*===========================================================================
 * 内部常量
 *===========================================================================*/

/**
 * @brief sysfs GPIO路径定义
 */
#define HAL_GPIO_SYSFS_BASE      "/sys/class/gpio"
#define HAL_GPIO_EXPORT_PATH     HAL_GPIO_SYSFS_BASE "/export"
#define HAL_GPIO_UNEXPORT_PATH   HAL_GPIO_SYSFS_BASE "/unexport"
#define HAL_GPIO_PATH_FMT        HAL_GPIO_SYSFS_BASE "/gpio%u"

/**
 * @brief GPIO sysfs文件路径最大长度
 */
#define HAL_GPIO_PATH_MAX        0x100

/*===========================================================================
 * 内部数据结构
 *===========================================================================*/

/**
 * @brief GPIO中断上下文（内部实现）
 *
 * 用于管理GPIO中断监听线程和回调函数
 */
typedef struct
{
	uint32_t gpio_num;                   /* GPIO引脚号 */
	hal_gpio_edge_t edge;                /* 中断触发模式 */
	hal_gpio_isr_callback_t callback;    /* 中断回调函数 */
	void *user_data;                     /* 用户数据 */
	osal_thread_t *thread;               /* 中断监听线程 */
	bool running;                        /* 运行标志 */
	int32_t value_fd;                    /* value文件描述符 */
} hal_gpio_isr_context_t;

/**
 * @brief GPIO上下文全局管理
 *
 * 由于GPIO可能需要全局状态管理（中断线程、导出状态等），
 * 具体实现可能需要全局数组或哈希表
 */

#endif /* HAL_GPIO_INTERNAL_H */
