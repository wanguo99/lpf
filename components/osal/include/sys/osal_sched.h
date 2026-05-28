/************************************************************************
 * OSAL - 实时调度封装
 *
 * 功能：
 * - 封装实时调度策略（SCHED_FIFO/SCHED_RR）
 * - 支持线程优先级设置
 * - 支持CPU亲和性设置
 * - 支持内存锁定
 *
 * 设计原则：
 * - 提供标准POSIX实时调度函数的封装
 * - 返回值统一为OSAL错误码
 * - 便于RTOS移植
 ************************************************************************/

#ifndef OSAL_SCHED_H
#define OSAL_SCHED_H

#include "osal/osal_platform.h"
#include "osal/osal_types.h"
#include "osal/sys/osal_thread.h"

/*
 * 调度策略（平台无关）
 */
#define OSAL_SCHED_OTHER    0   /* 普通调度策略（分时） */
#define OSAL_SCHED_FIFO     1   /* 先进先出实时调度 */
#define OSAL_SCHED_RR       2   /* 轮转实时调度 */

/*
 * 优先级范围
 * Linux: 1-99 (99最高)
 * FreeRTOS: 0-configMAX_PRIORITIES-1 (数值越大优先级越高)
 * 统一使用1-99，OSAL层负责转换
 */
#define OSAL_SCHED_PRIORITY_MIN     1
#define OSAL_SCHED_PRIORITY_MAX     99

/**
 * @brief 设置线程调度策略和优先级
 *
 * @param[in] thread    线程句柄（0表示当前线程）
 * @param[in] policy    调度策略（OSAL_SCHED_FIFO/OSAL_SCHED_RR/OSAL_SCHED_OTHER）
 * @param[in] priority  优先级（1-99，99最高）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER thread无效
 * @return OSAL_ERR_INVALID_PARAM policy或priority无效
 * @return OSAL_ERR_NO_PERMISSION 权限不足（需要root或CAP_SYS_NICE）
 * @return OSAL_ERR_GENERIC 设置失败
 *
 * @note Linux需要root权限或CAP_SYS_NICE能力
 * @note 实时调度策略会影响系统稳定性，使用时需谨慎
 */
int32_t OSAL_SchedSetPolicy(osal_thread_t thread, int32_t policy, int32_t priority);

/**
 * @brief 获取线程调度策略和优先级
 *
 * @param[in]  thread    线程句柄（0表示当前线程）
 * @param[out] policy    调度策略（可为NULL）
 * @param[out] priority  优先级（可为NULL）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER thread无效
 * @return OSAL_ERR_GENERIC 获取失败
 */
int32_t OSAL_SchedGetPolicy(osal_thread_t thread, int32_t *policy, int32_t *priority);

/**
 * @brief 设置线程优先级（保持当前调度策略）
 *
 * @param[in] thread    线程句柄（0表示当前线程）
 * @param[in] priority  优先级（1-99，99最高）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER thread无效
 * @return OSAL_ERR_INVALID_PARAM priority无效
 * @return OSAL_ERR_NO_PERMISSION 权限不足
 * @return OSAL_ERR_GENERIC 设置失败
 */
int32_t OSAL_SchedSetPriority(osal_thread_t thread, int32_t priority);

/**
 * @brief 获取线程优先级
 *
 * @param[in]  thread    线程句柄（0表示当前线程）
 * @param[out] priority  优先级
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER thread或priority无效
 * @return OSAL_ERR_GENERIC 获取失败
 */
int32_t OSAL_SchedGetPriority(osal_thread_t thread, int32_t *priority);

/**
 * @brief 设置线程CPU亲和性
 *
 * @param[in] thread    线程句柄（0表示当前线程）
 * @param[in] cpu_id    CPU编号（0-based）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER thread无效
 * @return OSAL_ERR_INVALID_PARAM cpu_id超出范围
 * @return OSAL_ERR_GENERIC 设置失败
 *
 * @note 用于将实时线程绑定到特定CPU，避免迁移开销
 */
int32_t OSAL_SchedSetAffinity(osal_thread_t thread, int32_t cpu_id);

/**
 * @brief 获取线程CPU亲和性
 *
 * @param[in]  thread    线程句柄（0表示当前线程）
 * @param[out] cpu_id    CPU编号
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER thread或cpu_id无效
 * @return OSAL_ERR_GENERIC 获取失败
 *
 * @note 如果线程可以在多个CPU上运行，返回第一个CPU编号
 */
int32_t OSAL_SchedGetAffinity(osal_thread_t thread, int32_t *cpu_id);

/**
 * @brief 锁定进程内存，防止页面交换
 *
 * @param[in] lock_all  true=锁定所有内存，false=仅锁定当前内存
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_NO_PERMISSION 权限不足（需要root或CAP_IPC_LOCK）
 * @return OSAL_ERR_NO_MEMORY 内存不足
 * @return OSAL_ERR_GENERIC 锁定失败
 *
 * @note 实时应用必须调用此函数，避免页面交换导致延迟
 * @note Linux需要root权限或CAP_IPC_LOCK能力
 * @note 锁定后内存使用量会增加，需确保有足够物理内存
 */
int32_t OSAL_MemLock(bool lock_all);

/**
 * @brief 解锁进程内存
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 解锁失败
 */
int32_t OSAL_MemUnlock(void);

/**
 * @brief 获取系统CPU数量
 *
 * @return CPU数量（>0）
 * @return -1 获取失败
 */
int32_t OSAL_GetCPUCount(void);

/**
 * @brief 获取调度策略的最小优先级
 *
 * @param[in] policy 调度策略
 *
 * @return 最小优先级（>0）
 * @return -1 获取失败
 */
int32_t OSAL_SchedGetPriorityMin(int32_t policy);

/**
 * @brief 获取调度策略的最大优先级
 *
 * @param[in] policy 调度策略
 *
 * @return 最大优先级（>0）
 * @return -1 获取失败
 */
int32_t OSAL_SchedGetPriorityMax(int32_t policy);

#endif /* OSAL_SCHED_H */
