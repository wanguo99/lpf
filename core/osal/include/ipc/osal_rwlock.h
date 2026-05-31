/************************************************************************
 * OSAL 读写锁接口
 *
 * 简单封装 pthread_rwlock，提供跨平台读写锁支持
 * 适用于读多写少的场景，允许多个读者同时访问，但写者独占
 ************************************************************************/

#ifndef OSAL_RWLOCK_H
#define OSAL_RWLOCK_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 读写锁句柄类型
 *
 * 不透明句柄，内部实现为 pthread_rwlock_t*
 */
typedef struct osal_rwlock_s osal_rwlock_t;

/**
 * @brief 创建读写锁
 *
 * @param[out] rwlock 读写锁句柄指针
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER rwlock 为 NULL
 * @return OSAL_ERR_NO_MEMORY 内存不足
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_RwlockCreate(osal_rwlock_t **rwlock);

/**
 * @brief 销毁读写锁
 *
 * @param[in] rwlock 读写锁句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER rwlock 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_RwlockDelete(osal_rwlock_t *rwlock);

/**
 * @brief 获取读锁
 *
 * @param[in] rwlock 读写锁句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER rwlock 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 *
 * @note 多个线程可以同时持有读锁
 */
int32_t OSAL_RwlockRdlock(osal_rwlock_t *rwlock);

/**
 * @brief 获取写锁
 *
 * @param[in] rwlock 读写锁句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER rwlock 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 *
 * @note 写锁是独占的，持有写锁时其他线程无法获取读锁或写锁
 */
int32_t OSAL_RwlockWrlock(osal_rwlock_t *rwlock);

/**
 * @brief 释放读写锁
 *
 * @param[in] rwlock 读写锁句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER rwlock 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_RwlockUnlock(osal_rwlock_t *rwlock);

/**
 * @brief 尝试获取读锁（非阻塞）
 *
 * @param[in] rwlock 读写锁句柄
 * @return OSAL_SUCCESS 成功获取锁
 * @return OSAL_ERR_INVALID_POINTER rwlock 为 NULL
 * @return OSAL_ERR_BUSY 锁已被占用
 * @return OSAL_ERR_GENERIC 系统调用失败
 *
 * @note 对应pthread_rwlock_tryrdlock，不会阻塞
 */
int32_t OSAL_RwlockTryRdlock(osal_rwlock_t *rwlock);

/**
 * @brief 尝试获取写锁（非阻塞）
 *
 * @param[in] rwlock 读写锁句柄
 * @return OSAL_SUCCESS 成功获取锁
 * @return OSAL_ERR_INVALID_POINTER rwlock 为 NULL
 * @return OSAL_ERR_BUSY 锁已被占用
 * @return OSAL_ERR_GENERIC 系统调用失败
 *
 * @note 对应pthread_rwlock_trywrlock，不会阻塞
 */
int32_t OSAL_RwlockTryWrlock(osal_rwlock_t *rwlock);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_RWLOCK_H */
