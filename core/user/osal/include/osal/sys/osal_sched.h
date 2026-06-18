/************************************************************************
 * OSAL - 实时调度接口
 *
 * 功能：
 * - 封装实时调度策略（OSAL_SCHED_FIFO/OSAL_SCHED_RR）
 * - 支持线程优先级设置
 * - 支持CPU亲和性设置
 ************************************************************************/

#ifndef OSAL_SCHED_H
#define OSAL_SCHED_H

#include <pthread.h>
#include <sched.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 调度类型定义
 *===========================================================================*/

#define OSAL_SCHED_OTHER SCHED_OTHER
#define OSAL_SCHED_FIFO SCHED_FIFO
#define OSAL_SCHED_RR SCHED_RR

#ifndef OSAL_SCHED_PARAM_T_DEFINED
#define OSAL_SCHED_PARAM_T_DEFINED
typedef struct sched_param osal_sched_param_t;
#endif

/*===========================================================================
 * 调度接口
 *===========================================================================*/

/**
 * @brief 设置线程调度策略和优先级
 *
 * @param[in] thread 线程 ID（osal_thread_self() 表示当前线程）
 * @param[in] policy 调度策略（OSAL_SCHED_FIFO/OSAL_SCHED_RR/OSAL_SCHED_OTHER）
 * @param[in] priority 优先级（1-99，99最高）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_set_sched_param(osal_thread_t thread, int32_t policy,
									int32_t priority);

/**
 * @brief 获取线程调度策略和优先级
 *
 * @param[in] thread 线程 ID
 * @param[out] policy 调度策略指针（可为 NULL）
 * @param[out] priority 优先级指针（可为 NULL）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_get_sched_param(osal_thread_t thread, int32_t *policy,
									int32_t *priority);

/**
 * @brief 设置线程 CPU 亲和性
 *
 * @param[in] thread 线程 ID
 * @param[in] cpu_id CPU 编号（0-based）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_set_affinity(osal_thread_t thread, int32_t cpu_id);

/**
 * @brief 获取线程 CPU 亲和性
 *
 * @param[in] thread 线程 ID
 * @param[out] cpu_id CPU 编号指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_get_affinity(osal_thread_t thread, int32_t *cpu_id);

/**
 * @brief 设置进程 CPU 亲和性
 *
 * @param[in] pid 进程 ID（0 表示当前进程）
 * @param[in] cpu_id CPU 编号（0-based）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sched_set_affinity(osal_pid_t pid, int32_t cpu_id);

/**
 * @brief 获取进程 CPU 亲和性
 *
 * @param[in] pid 进程 ID（0 表示当前进程）
 * @param[out] cpu_id CPU 编号指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sched_get_affinity(osal_pid_t pid, int32_t *cpu_id);

/**
 * @brief 设置进程调度策略
 *
 * @param[in] pid 进程 ID（0 表示当前进程）
 * @param[in] policy 调度策略
 * @param[in] param 调度参数
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sched_set_scheduler(osal_pid_t pid, int32_t policy,
								 const osal_sched_param_t *param);

/**
 * @brief 获取进程调度策略
 *
 * @param[in] pid 进程 ID（0 表示当前进程）
 * @return 调度策略，失败返回 -1
 */
int32_t osal_sched_get_scheduler(osal_pid_t pid);

/**
 * @brief 获取调度策略的最低优先级
 *
 * @param[in] policy 调度策略
 * @return 最低优先级，失败返回 -1
 */
int32_t osal_sched_get_priority_min(int32_t policy);

/**
 * @brief 获取调度策略的最高优先级
 *
 * @param[in] policy 调度策略
 * @return 最高优先级，失败返回 -1
 */
int32_t osal_sched_get_priority_max(int32_t policy);

/**
 * @brief 让出 CPU 时间片
 *
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SCHED_H */
