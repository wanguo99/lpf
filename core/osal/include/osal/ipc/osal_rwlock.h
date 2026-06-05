/************************************************************************
 * OSAL Reader-Writer Lock API
 *
 * Reader-writer lock for concurrent read access
 ************************************************************************/

#ifndef OSAL_RWLOCK_H
#define OSAL_RWLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 读写锁类型定义
 *===========================================================================*/

/**
 * @brief 读写锁句柄类型
 *
 * 不透明句柄，内部实现为 pthread_rwlock_t*
 */
typedef struct osal_rwlock_s osal_rwlock_t;

/*===========================================================================
 * 读写锁管理 API
 *===========================================================================*/

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
