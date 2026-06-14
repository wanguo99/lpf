/************************************************************************
 * OSAL - 实时调度封装（POSIX 薄封装）
 *
 * 功能：
 * - 封装实时调度策略（SCHED_FIFO/SCHED_RR）
 * - 支持线程优先级设置
 * - 支持CPU亲和性设置
 ************************************************************************/

#ifndef OSAL_SCHED_H
#define OSAL_SCHED_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * POSIX 调度薄封装
 *===========================================================================*/

/**
 * @brief 设置线程调度策略和优先级
 *
 * @param[in] thread 线程 ID（pthread_self() 表示当前线程）
 * @param[in] policy 调度策略（SCHED_FIFO/SCHED_RR/SCHED_OTHER）
 * @param[in] priority 优先级（1-99，99最高）
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_setschedparam(pthread_t thread, int32_t policy, int32_t priority);

/**
 * @brief 获取线程调度策略和优先级
 *
 * @param[in] thread 线程 ID
 * @param[out] policy 调度策略指针（可为 NULL）
 * @param[out] priority 优先级指针（可为 NULL）
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_getschedparam(pthread_t thread, int32_t *policy, int32_t *priority);

/**
 * @brief 设置线程 CPU 亲和性
 *
 * @param[in] thread 线程 ID
 * @param[in] cpu_id CPU 编号（0-based）
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_setaffinity_np(pthread_t thread, int32_t cpu_id);

/**
 * @brief 获取线程 CPU 亲和性
 *
 * @param[in] thread 线程 ID
 * @param[out] cpu_id CPU 编号指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_pthread_getaffinity_np(pthread_t thread, int32_t *cpu_id);

/**
 * @brief 让出 CPU 时间片
 *
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SCHED_H */
