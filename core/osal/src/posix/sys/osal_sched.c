/************************************************************************
 * OSAL - POSIX实时调度薄封装实现
 ************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "osal.h"
#include <pthread.h>
#include <sched.h>
#include <errno.h>

int32_t OSAL_pthread_setschedparam(osal_thread_t thread, int32_t policy, int32_t priority)
{
    osal_sched_param_t param;
    param.sched_priority = priority;

    return pthread_setschedparam(thread, policy, &param);
}

int32_t OSAL_pthread_getschedparam(osal_thread_t thread, int32_t *policy, int32_t *priority)
{
    osal_sched_param_t param;
    int pol;
    int ret;

    ret = pthread_getschedparam(thread, &pol, &param);
    if (ret == 0) {
        if (policy != NULL) {
            *policy = pol;
        }
        if (priority != NULL) {
            *priority = param.sched_priority;
        }
    }

    return ret;
}

int32_t OSAL_pthread_setaffinity_np(osal_thread_t thread, int32_t cpu_id)
{
#ifdef __linux__
    cpu_set_t cpuset;

    if (cpu_id < 0) {
        errno = EINVAL;
        return -1;
    }

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    return pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#else
    (void)thread;
    (void)cpu_id;
    errno = ENOSYS;
    return -1;
#endif
}

int32_t OSAL_pthread_getaffinity_np(osal_thread_t thread, int32_t *cpu_id)
{
#ifdef __linux__
    cpu_set_t cpuset;
    int ret;
    int i;

    if (cpu_id == NULL) {
        errno = EINVAL;
        return -1;
    }

    ret = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (ret == 0) {
        /* 返回第一个设置的 CPU */
        for (i = 0; i < CPU_SETSIZE; i++) {
            if (CPU_ISSET(i, &cpuset)) {
                *cpu_id = i;
                return 0;
            }
        }
    }

    return ret;
#else
    (void)thread;
    (void)cpu_id;
    errno = ENOSYS;
    return -1;
#endif
}

int32_t OSAL_sched_setaffinity(osal_pid_t pid, int32_t cpu_id)
{
#ifdef __linux__
    cpu_set_t cpuset;

    if (cpu_id < 0) {
        errno = EINVAL;
        return -1;
    }

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    return sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset);
#else
    (void)pid;
    (void)cpu_id;
    errno = ENOSYS;
    return -1;
#endif
}

int32_t OSAL_sched_getaffinity(osal_pid_t pid, int32_t *cpu_id)
{
#ifdef __linux__
    cpu_set_t cpuset;
    int ret;
    int i;

    if (cpu_id == NULL) {
        errno = EINVAL;
        return -1;
    }

    ret = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);
    if (ret == 0) {
        /* 返回第一个设置的 CPU */
        for (i = 0; i < CPU_SETSIZE; i++) {
            if (CPU_ISSET(i, &cpuset)) {
                *cpu_id = i;
                return 0;
            }
        }
    }

    return ret;
#else
    (void)pid;
    (void)cpu_id;
    errno = ENOSYS;
    return -1;
#endif
}

int32_t OSAL_sched_yield(void)
{
    return sched_yield();
}
