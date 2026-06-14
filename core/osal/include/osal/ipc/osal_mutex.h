/************************************************************************
 * OSAL Mutex API - POSIX 薄封装
 *
 * 直接暴露 POSIX pthread_mutex_t 类型
 ************************************************************************/

#ifndef OSAL_MUTEX_H
#define OSAL_MUTEX_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * POSIX 互斥锁薄封装
 *===========================================================================*/

/**
 * @brief 初始化互斥锁
 *
 * @param[in] mutex 互斥锁指针
 * @param[in] attr 互斥锁属性（NULL 表示默认）
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);

/**
 * @brief 销毁互斥锁
 *
 * @param[in] mutex 互斥锁指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_mutex_destroy(pthread_mutex_t *mutex);

/**
 * @brief 获取互斥锁（阻塞）
 *
 * @param[in] mutex 互斥锁指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_mutex_lock(pthread_mutex_t *mutex);

/**
 * @brief 尝试获取互斥锁（非阻塞）
 *
 * @param[in] mutex 互斥锁指针
 * @return 0 成功获取锁
 * @return -1 失败（EBUSY 表示锁已被占用）
 */
int32_t OSAL_pthread_mutex_trylock(pthread_mutex_t *mutex);

/**
 * @brief 带超时的获取互斥锁
 *
 * @param[in] mutex 互斥锁指针
 * @param[in] timeout_ms 超时时间（毫秒）
 * @return 0 成功获取锁
 * @return -1 失败（ETIMEDOUT 表示超时）
 */
int32_t OSAL_pthread_mutex_timedlock(pthread_mutex_t *mutex, uint32_t timeout_ms);

/**
 * @brief 释放互斥锁
 *
 * @param[in] mutex 互斥锁指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_mutex_unlock(pthread_mutex_t *mutex);

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
int32_t OSAL_pthread_mutexattr_init(pthread_mutexattr_t *attr);

/**
 * @brief 销毁互斥锁属性
 *
 * @param[in] attr 属性指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

/**
 * @brief 设置互斥锁类型
 *
 * @param[in] attr 属性指针
 * @param[in] type 互斥锁类型（PTHREAD_MUTEX_NORMAL/PTHREAD_MUTEX_RECURSIVE等）
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_mutexattr_settype(pthread_mutexattr_t *attr, int32_t type);

/**
 * @brief 获取互斥锁类型
 *
 * @param[in] attr 属性指针
 * @param[out] type 互斥锁类型指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int32_t *type);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_MUTEX_H */
