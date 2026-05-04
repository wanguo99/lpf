/************************************************************************
 * OSAL POSIX实现 - 互斥锁
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <stdlib.h>

struct osal_mutex_s
{
    pthread_mutex_t mutex;
};

int32_t OSAL_MutexCreate(osal_mutex_t **mutex)
{
    if (NULL == mutex)
        return OSAL_ERR_INVALID_POINTER;

    osal_mutex_t *new_mutex = (osal_mutex_t *)malloc(sizeof(osal_mutex_t));
    if (NULL == new_mutex)
        return OSAL_ERR_GENERIC;

    if (0 != pthread_mutex_init(&new_mutex->mutex, NULL))
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
