/************************************************************************
 * OSAL Reader-Writer Lock API
 *
 * 面向调用方提供统一读写锁接口；平台相关类型由当前 OSAL 后端映射。
 ************************************************************************/

#ifndef OSAL_RWLOCK_H
#define OSAL_RWLOCK_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 读写锁类型定义
 *===========================================================================*/

typedef pthread_rwlock_t osal_rwlock_t;
typedef pthread_rwlockattr_t osal_rwlock_attr_t;

/*===========================================================================
 * 读写锁接口
 *===========================================================================*/

/**
 * @brief 初始化读写锁
 *
 * @param[in] rwlock 读写锁指针
 * @param[in] attr 读写锁属性（NULL 表示默认）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_rwlock_init(osal_rwlock_t *rwlock, const osal_rwlock_attr_t *attr);

/**
 * @brief 销毁读写锁
 *
 * @param[in] rwlock 读写锁指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_rwlock_destroy(osal_rwlock_t *rwlock);

/**
 * @brief 获取读锁（阻塞）
 *
 * @param[in] rwlock 读写锁指针
 * @return 0 成功
 * @return -1 失败
 *
 * @note 多个线程可以同时持有读锁
 */
int32_t osal_rwlock_read_lock(osal_rwlock_t *rwlock);

/**
 * @brief 获取写锁（阻塞）
 *
 * @param[in] rwlock 读写锁指针
 * @return 0 成功
 * @return -1 失败
 *
 * @note 写锁是独占的，持有写锁时其他线程无法获取读锁或写锁
 */
int32_t osal_rwlock_write_lock(osal_rwlock_t *rwlock);

/**
 * @brief 尝试获取读锁（非阻塞）
 *
 * @param[in] rwlock 读写锁指针
 * @return 0 成功获取锁
 * @return -1 失败（EBUSY 表示锁已被占用）
 */
int32_t osal_rwlock_try_read_lock(osal_rwlock_t *rwlock);

/**
 * @brief 尝试获取写锁（非阻塞）
 *
 * @param[in] rwlock 读写锁指针
 * @return 0 成功获取锁
 * @return -1 失败（EBUSY 表示锁已被占用）
 */
int32_t osal_rwlock_try_write_lock(osal_rwlock_t *rwlock);

/**
 * @brief 释放读写锁
 *
 * @param[in] rwlock 读写锁指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_rwlock_unlock(osal_rwlock_t *rwlock);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_RWLOCK_H */
