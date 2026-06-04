/************************************************************************
 * OSAL - 实时调度封装（POSIX实现）
 ************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE  /* 需要CPU_SET等宏 */
#endif

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "osal/osal.h"


/* 简化错误码别名 */
#define OSAL_ERR_INVALID_PARAM   OSAL_EINVAL
#define OSAL_ERR_NO_PERMISSION   OSAL_EPERM
#define OSAL_ERR_GENERIC         OSAL_EIO

/*
 * 将OSAL调度策略转换为POSIX调度策略
 */
static int osal_to_posix_policy(int32_t osal_policy)
{
    switch (osal_policy) {
        case OSAL_SCHED_OTHER:
            return SCHED_OTHER;
        case OSAL_SCHED_FIFO:
            return SCHED_FIFO;
        case OSAL_SCHED_RR:
            return SCHED_RR;
        default:
            return -1;
    }
}

/*
 * 将POSIX调度策略转换为OSAL调度策略
 */
static int32_t posix_to_osal_policy(int posix_policy)
{
    switch (posix_policy) {
        case SCHED_OTHER:
            return OSAL_SCHED_OTHER;
        case SCHED_FIFO:
            return OSAL_SCHED_FIFO;
        case SCHED_RR:
            return OSAL_SCHED_RR;
        default:
            return -1;
    }
}

/*
 * 设置线程调度策略和优先级
 */
int32_t OSAL_SchedSetPolicy(osal_thread_t thread, int32_t policy, int32_t priority)
{
    pthread_t pthread;
    struct sched_param param;
    int posix_policy;
    int ret;

    /* 参数校验 */
    if (priority < OSAL_SCHED_PRIORITY_MIN || priority > OSAL_SCHED_PRIORITY_MAX) {
        return OSAL_ERR_INVALID_PARAM;
    }

    posix_policy = osal_to_posix_policy(policy);
    if (posix_policy < 0) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 0表示当前线程 */
    if (thread == 0) {
        pthread = pthread_self();
    } else {
        pthread = (pthread_t)thread;
    }

    /* 设置优先级 */
    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;

    ret = pthread_setschedparam(pthread, posix_policy, &param);
    if (ret != 0) {
        if (ret == EPERM) {
            return OSAL_ERR_NO_PERMISSION;
        }
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/*
 * 获取线程调度策略和优先级
 */
int32_t OSAL_SchedGetPolicy(osal_thread_t thread, int32_t *policy, int32_t *priority)
{
    pthread_t pthread;
    struct sched_param param;
    int posix_policy;
    int ret;

    /* 0表示当前线程 */
    if (thread == 0) {
        pthread = pthread_self();
    } else {
        pthread = (pthread_t)thread;
    }

    ret = pthread_getschedparam(pthread, &posix_policy, &param);
    if (ret != 0) {
        return OSAL_ERR_GENERIC;
    }

    if (policy != NULL) {
        *policy = posix_to_osal_policy(posix_policy);
    }

    if (priority != NULL) {
        *priority = param.sched_priority;
    }

    return OSAL_SUCCESS;
}

/*
 * 设置线程优先级（保持当前调度策略）
 */
int32_t OSAL_SchedSetPriority(osal_thread_t thread, int32_t priority)
{
    int32_t current_policy;
    int32_t ret;

    /* 参数校验 */
    if (priority < OSAL_SCHED_PRIORITY_MIN || priority > OSAL_SCHED_PRIORITY_MAX) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 获取当前调度策略 */
    ret = OSAL_SchedGetPolicy(thread, &current_policy, NULL);
    if (ret != OSAL_SUCCESS) {
        return ret;
    }

    /* 设置新优先级 */
    return OSAL_SchedSetPolicy(thread, current_policy, priority);
}

/*
 * 获取线程优先级
 */
int32_t OSAL_SchedGetPriority(osal_thread_t thread, int32_t *priority)
{
    if (priority == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    return OSAL_SchedGetPolicy(thread, NULL, priority);
}

/*
 * 设置线程CPU亲和性
 */
int32_t OSAL_SchedSetAffinity(osal_thread_t thread, int32_t cpu_id)
{
#ifdef __linux__
    pthread_t pthread;
    cpu_set_t cpuset;
    int ret;
    int cpu_count;

    /* 参数校验 */
    cpu_count = OSAL_GetCPUCount();
    if (cpu_id < 0 || cpu_id >= cpu_count) {
        return OSAL_ERR_INVALID_PARAM;
    }

    /* 0表示当前线程 */
    if (thread == 0) {
        pthread = pthread_self();
    } else {
        pthread = (pthread_t)thread;
    }

    /* 设置CPU亲和性 */
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);

    ret = pthread_setaffinity_np(pthread, sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
#else
    /* macOS不支持pthread_setaffinity_np */
    (void)thread;
    (void)cpu_id;
    return OSAL_ERR_NOT_IMPLEMENTED;
#endif
}

/*
 * 获取线程CPU亲和性
 */
int32_t OSAL_SchedGetAffinity(osal_thread_t thread, int32_t *cpu_id)
{
#ifdef __linux__
    pthread_t pthread;
    cpu_set_t cpuset;
    int ret;
    int i;
    int cpu_count;

    if (cpu_id == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    /* 0表示当前线程 */
    if (thread == 0) {
        pthread = pthread_self();
    } else {
        pthread = (pthread_t)thread;
    }

    /* 获取CPU亲和性 */
    CPU_ZERO(&cpuset);
    ret = pthread_getaffinity_np(pthread, sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
        return OSAL_ERR_GENERIC;
    }

    /* 返回第一个设置的CPU */
    cpu_count = OSAL_GetCPUCount();
    for (i = 0; i < cpu_count; i++) {
        if (CPU_ISSET(i, &cpuset)) {
            *cpu_id = i;
            return OSAL_SUCCESS;
        }
    }

    return OSAL_ERR_GENERIC;
#else
    /* macOS不支持pthread_getaffinity_np */
    if (cpu_id == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }
    (void)thread;
    *cpu_id = 0;  /* 默认返回CPU 0 */
    return OSAL_ERR_NOT_IMPLEMENTED;
#endif
}

/*
 * 锁定进程内存，防止页面交换
 */
int32_t OSAL_MemLock(bool lock_all)
{
    int ret;

    if (lock_all) {
        /* 锁定所有内存（包括未来分配的） */
        ret = mlockall(MCL_CURRENT | MCL_FUTURE);
    } else {
        /* 仅锁定当前内存 */
        ret = mlockall(MCL_CURRENT);
    }

    if (ret != 0) {
        if (errno == EPERM) {
            return OSAL_ERR_PERMISSION;
        } else if (errno == ENOMEM) {
            return OSAL_ERR_NO_MEMORY;
        }
        /* 其他错误返回通用错误 */
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/*
 * 解锁进程内存
 */
int32_t OSAL_MemUnlock(void)
{
    int ret;

    ret = munlockall();
    if (ret != 0) {
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/*
 * 获取系统CPU数量
 */
int32_t OSAL_GetCPUCount(void)
{
    long cpu_count;

    /* 使用_SC_NPROCESSORS_ONLN获取在线CPU数量 */
    /* macOS和Linux都支持这个常量 */
    cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count < 0) {
        /* 如果失败，返回默认值1 */
        return 1;
    }

    return (int32_t)cpu_count;
}

/*
 * 获取调度策略的最小优先级
 */
int32_t OSAL_SchedGetPriorityMin(int32_t policy)
{
    int posix_policy;
    int min_priority;

    posix_policy = osal_to_posix_policy(policy);
    if (posix_policy < 0) {
        return -1;
    }

    min_priority = sched_get_priority_min(posix_policy);
    if (min_priority < 0) {
        return -1;
    }

    return min_priority;
}

/*
 * 获取调度策略的最大优先级
 */
int32_t OSAL_SchedGetPriorityMax(int32_t policy)
{
    int posix_policy;
    int max_priority;

    posix_policy = osal_to_posix_policy(policy);
    if (posix_policy < 0) {
        return -1;
    }

    max_priority = sched_get_priority_max(posix_policy);
    if (max_priority < 0) {
        return -1;
    }

    return max_priority;
}
