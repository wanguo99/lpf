/************************************************************************
 * OSAL Thread API
 *
 * Thread creation, management, and synchronization primitives
 ************************************************************************/

#ifndef OSAL_THREAD_API_H
#define OSAL_THREAD_API_H

#include "osal_types_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 线程类型定义
 *===========================================================================*/

/**
 * @brief 线程句柄类型（平台无关）
 */
typedef uint64_t osal_thread_t;

/**
 * @brief 线程函数类型
 */
typedef void *(*osal_thread_func_t)(void *arg);

/**
 * @brief 线程属性
 */
typedef struct {
	size_t stack_size;      /* 栈大小，0表示默认 */
	bool   detached;        /* 是否分离 */
	int32_t inherit_sched;  /* 是否继承调度属性（0=不继承，1=继承） */
	int32_t sched_policy;   /* 调度策略（SCHED_OTHER/FIFO/RR） */
	int32_t sched_priority; /* 调度优先级（1-99） */
} osal_thread_attr_t;

/*===========================================================================
 * 线程管理 API
 *===========================================================================*/

/**
 * @brief 创建线程（简化接口，默认属性）
 *
 * @param[out] thread 线程句柄
 * @param[in]  func   线程函数
 * @param[in]  arg    线程参数
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER thread或func为NULL
 * @return OSAL_ERR_GENERIC 创建失败
 */
int32_t OSAL_ThreadCreate(osal_thread_t *thread, osal_thread_func_t func, void *arg);

/**
 * @brief 创建线程（扩展接口，自定义属性）
 *
 * @param[out] thread 线程句柄
 * @param[in]  attr   线程属性
 * @param[in]  func   线程函数
 * @param[in]  arg    线程参数
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER thread或func为NULL
 * @return OSAL_ERR_GENERIC 创建失败
 *
 * @note 可以设置栈大小、分离状态、调度策略等
 */
int32_t OSAL_ThreadCreateEx(osal_thread_t *thread,
                             const osal_thread_attr_t *attr,
                             osal_thread_func_t func,
                             void *arg);

/**
 * @brief 等待线程结束
 *
 * @param[in] thread 线程句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t OSAL_ThreadJoin(osal_thread_t thread);

/**
 * @brief 获取当前线程句柄
 *
 * @return 当前线程句柄
 */
osal_thread_t OSAL_ThreadSelf(void);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_THREAD_API_H */
