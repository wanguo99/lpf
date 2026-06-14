/************************************************************************
 * OSAL POSIX实现 - 信号量薄封装
 ************************************************************************/

#include "osal.h"
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

int32_t OSAL_sem_init(sem_t *sem, int32_t pshared, uint32_t value)
{
    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    return sem_init(sem, pshared, value);
}

int32_t OSAL_sem_destroy(sem_t *sem)
{
    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    return sem_destroy(sem);
}

int32_t OSAL_sem_wait(sem_t *sem)
{
    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    return sem_wait(sem);
}

int32_t OSAL_sem_trywait(sem_t *sem)
{
    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    return sem_trywait(sem);
}

int32_t OSAL_sem_timedwait(sem_t *sem, uint32_t timeout_ms)
{
    struct timespec ts;

    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* 非阻塞模式 */
    if (timeout_ms == 0) {
        return sem_trywait(sem);
    }

#ifdef __APPLE__
    /* macOS 不支持 sem_timedwait，使用轮询模拟 */
    struct timeval start, now;
    uint32_t elapsed_ms;

    gettimeofday(&start, NULL);

    while (1) {
        if (sem_trywait(sem) == 0) {
            return 0;
        }

        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return -1;
        }

        gettimeofday(&now, NULL);
        elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 +
                     (now.tv_usec - start.tv_usec) / 1000;

        if (elapsed_ms >= timeout_ms) {
            errno = ETIMEDOUT;
            return -1;
        }

        /* 短暂休眠避免 CPU 占用过高 */
        usleep(1000);  /* 1ms */
    }
#else
    /* Linux 使用 sem_timedwait */
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return -1;
    }

    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    return sem_timedwait(sem, &ts);
#endif
}

int32_t OSAL_sem_post(sem_t *sem)
{
    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    return sem_post(sem);
}

int32_t OSAL_sem_getvalue(sem_t *sem, int32_t *value)
{
    int val;

    if (sem == NULL || value == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (sem_getvalue(sem, &val) != 0) {
        return -1;
    }

    *value = val;
    return 0;
}
