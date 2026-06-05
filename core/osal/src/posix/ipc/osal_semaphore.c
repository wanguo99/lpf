/************************************************************************
 * OSAL POSIX实现 - 信号量
 ************************************************************************/

#include "osal.h"
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __APPLE__
/* macOS不支持sem_init/sem_destroy/sem_timedwait，使用命名信号量 */
struct osal_semaphore_s
{
    sem_t *sem;
    char name[64];
};
#else
/* Linux使用未命名信号量 */
struct osal_semaphore_s
{
    sem_t sem;
};
#endif

int32_t OSAL_SemaphoreCreate(osal_semaphore_t **sem, uint32_t initial_value)
{
    osal_semaphore_t *new_sem;

    if (NULL == sem)
        return OSAL_ERR_INVALID_POINTER;

    if (initial_value > (uint32_t)INT32_MAX)
        return OSAL_ERR_INVALID_SEM_VALUE;

    new_sem = (osal_semaphore_t *)malloc(sizeof(osal_semaphore_t));
    if (NULL == new_sem)
        return OSAL_ERR_GENERIC;

#ifdef __APPLE__
    /* macOS使用命名信号量 */
    static uint32_t sem_counter = 0;
    snprintf(new_sem->name, sizeof(new_sem->name), "/osal_sem_%d_%u", getpid(), sem_counter++);

    /* 先尝试删除可能存在的同名信号量 */
    sem_unlink(new_sem->name);

    new_sem->sem = sem_open(new_sem->name, O_CREAT | O_EXCL, 0600, initial_value);
    if (new_sem->sem == SEM_FAILED)
    {
        free(new_sem);
        return OSAL_ERR_GENERIC;
    }
#else
    /* Linux使用未命名信号量 */
    if (0 != sem_init(&new_sem->sem, 0, initial_value))
    {
        free(new_sem);
        return OSAL_ERR_GENERIC;
    }
#endif

    *sem = new_sem;
    return OSAL_SUCCESS;
}

int32_t OSAL_SemaphoreDelete(osal_semaphore_t *sem)
{
    if (NULL == sem)
        return OSAL_ERR_INVALID_POINTER;

#ifdef __APPLE__
    sem_close(sem->sem);
    sem_unlink(sem->name);
#else
    sem_destroy(&sem->sem);
#endif
    free(sem);
    return OSAL_SUCCESS;
}

int32_t OSAL_SemaphoreWait(osal_semaphore_t *sem)
{
    if (NULL == sem)
        return OSAL_ERR_INVALID_POINTER;

#ifdef __APPLE__
    if (0 != sem_wait(sem->sem))
        return OSAL_ERR_GENERIC;
#else
    if (0 != sem_wait(&sem->sem))
        return OSAL_ERR_GENERIC;
#endif

    return OSAL_SUCCESS;
}

int32_t OSAL_SemaphoreTimedWait(osal_semaphore_t *sem, uint32_t timeout_ms)
{
#ifdef __APPLE__
    struct timeval start, now;
    uint32_t elapsed_ms;
#else
    struct timespec ts;
#endif

    if (NULL == sem)
        return OSAL_ERR_INVALID_POINTER;

    if (0 == timeout_ms)
    {
        /* 非阻塞模式 */
#ifdef __APPLE__
        if (0 != sem_trywait(sem->sem))
#else
        if (0 != sem_trywait(&sem->sem))
#endif
        {
            if (EAGAIN == errno || EWOULDBLOCK == errno)
                return OSAL_ERR_TIMEOUT;
            return OSAL_ERR_GENERIC;
        }
        return OSAL_SUCCESS;
    }

#ifdef __APPLE__
    /* macOS不支持sem_timedwait，使用轮询方式 */
    gettimeofday(&start, NULL);

    while (1)
    {
        if (0 == sem_trywait(sem->sem))
            return OSAL_SUCCESS;

        if (errno != EAGAIN && errno != EWOULDBLOCK)
            return OSAL_ERR_GENERIC;

        gettimeofday(&now, NULL);
        elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                     (now.tv_usec - start.tv_usec) / 1000;

        if (elapsed_ms >= timeout_ms)
            return OSAL_ERR_TIMEOUT;

        /* 短暂休眠，避免CPU占用过高 */
        usleep(1000);  /* 1ms */
    }
#else
    /* Linux使用sem_timedwait */
    /* 计算超时时间点 */
    if (0 != clock_gettime(CLOCK_REALTIME, &ts))
        return OSAL_ERR_GENERIC;

    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    if (0 != sem_timedwait(&sem->sem, &ts))
    {
        if (ETIMEDOUT == errno)
            return OSAL_ERR_TIMEOUT;
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
#endif
}

int32_t OSAL_SemaphorePost(osal_semaphore_t *sem)
{
    if (NULL == sem)
        return OSAL_ERR_INVALID_POINTER;

#ifdef __APPLE__
    if (0 != sem_post(sem->sem))
        return OSAL_ERR_GENERIC;
#else
    if (0 != sem_post(&sem->sem))
        return OSAL_ERR_GENERIC;
#endif

    return OSAL_SUCCESS;
}
