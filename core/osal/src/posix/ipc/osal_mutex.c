/************************************************************************
 * OSAL POSIX实现 - 互斥锁
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

struct osal_mutex_s
{
    pthread_mutex_t mutex;
};

int32_t OSAL_MutexCreate(osal_mutex_t **mutex)
{
    osal_mutex_t *new_mutex;

    if (NULL == mutex)
        return OSAL_ERR_INVALID_POINTER;

    new_mutex = (osal_mutex_t *)malloc(sizeof(osal_mutex_t));
    if (NULL == new_mutex)
        return OSAL_ERR_NO_MEMORY;

    if (0 != pthread_mutex_init(&new_mutex->mutex, NULL))
    {
        free(new_mutex);
        return OSAL_ERR_GENERIC;
    }

    *mutex = new_mutex;
    return OSAL_SUCCESS;
}

int32_t OSAL_MutexCreateEx(osal_mutex_t **mutex, const osal_mutex_attr_t *attr)
{
    osal_mutex_t *new_mutex;
    pthread_mutexattr_t pthread_attr;
    int ret;

    if (NULL == mutex)
        return OSAL_ERR_INVALID_POINTER;

    new_mutex = (osal_mutex_t *)malloc(sizeof(osal_mutex_t));
    if (NULL == new_mutex)
        return OSAL_ERR_NO_MEMORY;

    pthread_mutexattr_init(&pthread_attr);

    if (attr != NULL && attr->type == OSAL_MUTEX_RECURSIVE)
    {
        pthread_mutexattr_settype(&pthread_attr, PTHREAD_MUTEX_RECURSIVE);
    }

    ret = pthread_mutex_init(&new_mutex->mutex, &pthread_attr);
    pthread_mutexattr_destroy(&pthread_attr);

    if (0 != ret)
    {
        free(new_mutex);
        return OSAL_ERR_GENERIC;
    }

    *mutex = new_mutex;
    return OSAL_SUCCESS;
}

int32_t OSAL_MutexDelete(osal_mutex_t *mutex)
{
    if (NULL == mutex)
        return OSAL_ERR_INVALID_POINTER;

    pthread_mutex_destroy(&mutex->mutex);
    free(mutex);
    return OSAL_SUCCESS;
}

int32_t OSAL_MutexLock(osal_mutex_t *mutex)
{
    if (NULL == mutex)
        return OSAL_ERR_INVALID_POINTER;

    if (0 != pthread_mutex_lock(&mutex->mutex))
        return OSAL_ERR_GENERIC;

    return OSAL_SUCCESS;
}

int32_t OSAL_MutexUnlock(osal_mutex_t *mutex)
{
    if (NULL == mutex)
        return OSAL_ERR_INVALID_POINTER;

    if (0 != pthread_mutex_unlock(&mutex->mutex))
        return OSAL_ERR_GENERIC;

    return OSAL_SUCCESS;
}

int32_t OSAL_MutexTryLock(osal_mutex_t *mutex)
{
    int ret;

    if (NULL == mutex)
        return OSAL_ERR_INVALID_POINTER;

    ret = pthread_mutex_trylock(&mutex->mutex);
    if (0 == ret)
        return OSAL_SUCCESS;

    if (EBUSY == ret)
        return OSAL_ERR_BUSY;

    return OSAL_ERR_GENERIC;
}

int32_t OSAL_MutexTimedLock(osal_mutex_t *mutex, uint32_t timeout_ms)
{
    struct timespec ts;
    int ret;

    if (NULL == mutex)
        return OSAL_ERR_INVALID_POINTER;

    if (0 != clock_gettime(CLOCK_REALTIME, &ts))
        return OSAL_ERR_GENERIC;

    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000)
    {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    ret = pthread_mutex_timedlock(&mutex->mutex, &ts);
    if (0 == ret)
        return OSAL_SUCCESS;
    if (ETIMEDOUT == ret)
        return OSAL_ERR_TIMEOUT;

    return OSAL_ERR_GENERIC;
}
