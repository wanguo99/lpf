/************************************************************************
 * OSAL Mutex API
 *
 * 面向调用方提供统一互斥锁接口；平台相关类型由当前 OSAL 后端映射。
 ************************************************************************/

#ifndef OSAL_MUTEX_H
#define OSAL_MUTEX_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 互斥锁类型定义
 *===========================================================================*/

#define OSAL_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define OSAL_MUTEX_NORMAL PTHREAD_MUTEX_NORMAL
#define OSAL_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE
#define OSAL_MUTEX_PRIO_INHERIT PTHREAD_PRIO_INHERIT

typedef pthread_mutex_t osal_mutex_t;
typedef pthread_mutexattr_t osal_mutex_attr_t;

/*===========================================================================
 * 互斥锁接口
 *===========================================================================*/

/**
 * @brief 初始化互斥锁
 *
 * @param[in] mutex 互斥锁指针
 * @param[in] attr 互斥锁属性（NULL 表示默认）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_init(osal_mutex_t *mutex, const osal_mutex_attr_t *attr);

/**
 * @brief 销毁互斥锁
 *
 * @param[in] mutex 互斥锁指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_destroy(osal_mutex_t *mutex);

/**
 * @brief 获取互斥锁（阻塞）
 *
 * @param[in] mutex 互斥锁指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_lock(osal_mutex_t *mutex);

/**
 * @brief 尝试获取互斥锁（非阻塞）
 *
 * @param[in] mutex 互斥锁指针
 * @return 0 成功获取锁
 * @return -1 失败（EBUSY 表示锁已被占用）
 */
int32_t osal_mutex_try_lock(osal_mutex_t *mutex);

/**
 * @brief 带超时的获取互斥锁
 *
 * @param[in] mutex 互斥锁指针
 * @param[in] timeout_ms 超时时间（毫秒）
 * @return 0 成功获取锁
 * @return -1 失败（ETIMEDOUT 表示超时）
 */
int32_t osal_mutex_timed_lock(osal_mutex_t *mutex, uint32_t timeout_ms);

/**
 * @brief 释放互斥锁
 *
 * @param[in] mutex 互斥锁指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_unlock(osal_mutex_t *mutex);

/*===========================================================================
 * 互斥锁属性管理（可选）
 *===========================================================================*/

/**
 * @brief 初始化互斥锁属性
 *
 * @param[in] attr 属性指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_attr_init(osal_mutex_attr_t *attr);

/**
 * @brief 销毁互斥锁属性
 *
 * @param[in] attr 属性指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_attr_destroy(osal_mutex_attr_t *attr);

/**
 * @brief 设置互斥锁类型
 *
 * @param[in] attr 属性指针
 * @param[in] type 互斥锁类型（OSAL_MUTEX_NORMAL/
 * OSAL_MUTEX_RECURSIVE 等）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_attr_set_type(osal_mutex_attr_t *attr, int32_t type);

/**
 * @brief 设置互斥锁协议
 *
 * @param[in] attr 属性指针
 * @param[in] protocol 协议（OSAL_MUTEX_PRIO_INHERIT 等）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_attr_set_protocol(osal_mutex_attr_t *attr, int32_t protocol);

/**
 * @brief 获取互斥锁类型
 *
 * @param[in] attr 属性指针
 * @param[out] type 互斥锁类型指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_mutex_attr_get_type(const osal_mutex_attr_t *attr, int32_t *type);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_MUTEX_H */
