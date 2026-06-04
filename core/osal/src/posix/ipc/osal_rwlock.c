/************************************************************************
 * OSAL 读写锁实现
 *
 * 基于 pthread_rwlock_t 实现
 ************************************************************************/

#include "osal/osal.h"
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

/**
 * @brief 读写锁内部结构
 */
struct osal_rwlock_s {
    pthread_rwlock_t rwlock;
};

/**
 * @brief 创建读写锁
 */
int32_t OSAL_RwlockCreate(osal_rwlock_t **rwlock)
{
    int ret;
    osal_rwlock_t *lock;

    if (NULL == rwlock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    lock = (osal_rwlock_t *)malloc(sizeof(osal_rwlock_t));
    if (NULL == lock) {
        return OSAL_ERR_NO_MEMORY;
    }

    ret = pthread_rwlock_init(&lock->rwlock, NULL);
    if (0 != ret) {
        free(lock);
        return OSAL_ERR_GENERIC;
    }

    *rwlock = lock;
    return OSAL_SUCCESS;
}

/**
 * @brief 销毁读写锁
 */
int32_t OSAL_RwlockDelete(osal_rwlock_t *rwlock)
{
    int ret;

    if (NULL == rwlock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    ret = pthread_rwlock_destroy(&rwlock->rwlock);
    free(rwlock);

    return (0 == ret) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * @brief 获取读锁
 */
int32_t OSAL_RwlockRdlock(osal_rwlock_t *rwlock)
{
    int ret;

    if (NULL == rwlock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    ret = pthread_rwlock_rdlock(&rwlock->rwlock);
    return (0 == ret) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * @brief 获取写锁
 */
int32_t OSAL_RwlockWrlock(osal_rwlock_t *rwlock)
{
    int ret;

    if (NULL == rwlock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    ret = pthread_rwlock_wrlock(&rwlock->rwlock);
    return (0 == ret) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * @brief 释放读写锁
 */
int32_t OSAL_RwlockUnlock(osal_rwlock_t *rwlock)
{
    int ret;

    if (NULL == rwlock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    ret = pthread_rwlock_unlock(&rwlock->rwlock);
    return (0 == ret) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

/**
 * @brief 尝试获取读锁（非阻塞）
 */
int32_t OSAL_RwlockTryRdlock(osal_rwlock_t *rwlock)
{
    int ret;

    if (NULL == rwlock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    ret = pthread_rwlock_tryrdlock(&rwlock->rwlock);
    if (0 == ret) {
        return OSAL_SUCCESS;
    } else if (EBUSY == ret) {
        return OSAL_ERR_BUSY;
    } else {
        return OSAL_ERR_GENERIC;
    }
}

/**
 * @brief 尝试获取写锁（非阻塞）
 */
int32_t OSAL_RwlockTryWrlock(osal_rwlock_t *rwlock)
{
    int ret;

    if (NULL == rwlock) {
        return OSAL_ERR_INVALID_POINTER;
    }

    ret = pthread_rwlock_trywrlock(&rwlock->rwlock);
    if (0 == ret) {
        return OSAL_SUCCESS;
    } else if (EBUSY == ret) {
        return OSAL_ERR_BUSY;
    } else {
        return OSAL_ERR_GENERIC;
    }
}
