/************************************************************************
 * OSAL 条件变量实现（POSIX）
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

struct osal_cond_s {
    pthread_cond_t cond;
};

struct osal_mutex_s {
    pthread_mutex_t mutex;
};

int32_t OSAL_CondCreate(osal_cond_t **cond)
{
    if (cond == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    osal_cond_t *new_cond = (osal_cond_t *)malloc(sizeof(osal_cond_t));
    if (new_cond == NULL) {
        return OSAL_ERR_GENERIC;
    }

    int ret = pthread_cond_init(&new_cond->cond, NULL);
    if (ret != 0) {
        free(new_cond);
        return OSAL_ERR_GENERIC;
    }

    *cond = new_cond;
    return OSAL_SUCCESS;
}

int32_t OSAL_CondDelete(osal_cond_t *cond)
{
    if (cond == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    int ret = pthread_cond_destroy(&cond->cond);
    free(cond);

    return (ret == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t OSAL_CondWait(osal_cond_t *cond, osal_mutex_t *mutex)
{
    if (cond == NULL || mutex == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    int ret = pthread_cond_wait(&cond->cond, &mutex->mutex);
    return (ret == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t OSAL_CondTimedWait(osal_cond_t *cond, osal_mutex_t *mutex, uint32_t timeout_ms)
{
    if (cond == NULL || mutex == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return OSAL_ERR_GENERIC;
    }

    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    int ret = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &ts);
    if (ret == 0) {
        return OSAL_SUCCESS;
    } else if (ret == ETIMEDOUT) {
        return OSAL_ERR_TIMEOUT;
    } else {
        return OSAL_ERR_GENERIC;
    }
}

int32_t OSAL_CondSignal(osal_cond_t *cond)
{
    if (cond == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    int ret = pthread_cond_signal(&cond->cond);
    return (ret == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}

int32_t OSAL_CondBroadcast(osal_cond_t *cond)
{
    if (cond == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    int ret = pthread_cond_broadcast(&cond->cond);
    return (ret == 0) ? OSAL_SUCCESS : OSAL_ERR_GENERIC;
}
