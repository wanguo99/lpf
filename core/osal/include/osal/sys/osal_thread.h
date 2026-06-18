/************************************************************************
 * OSAL Thread API
 *
 * 面向调用方提供统一线程接口；平台相关类型由当前 OSAL 后端映射。
 ************************************************************************/

#ifndef OSAL_THREAD_H
#define OSAL_THREAD_H

#include <pthread.h>
#include <sched.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 线程类型定义
 *===========================================================================*/

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
/* 调度参数类型（与 osal_sched.h 共享） */
#ifndef OSAL_SCHED_PARAM_T_DEFINED
#define OSAL_SCHED_PARAM_T_DEFINED
typedef struct sched_param osal_sched_param_t;
#endif

typedef pthread_t osal_thread_t;
typedef pthread_attr_t osal_thread_attr_t;

#define OSAL_THREAD_CREATE_JOINABLE PTHREAD_CREATE_JOINABLE
#define OSAL_THREAD_CREATE_DETACHED PTHREAD_CREATE_DETACHED
#else
/* 其他平台（RTOS 等）- 需要提供对应的类型定义 */
#error "Unsupported platform - please define thread types for your platform"
#endif

/*===========================================================================
 * 线程接口
 *===========================================================================*/

/**
 * @brief 创建线程
 *
 * @param[out] thread 线程指针
 * @param[in] attr 线程属性（NULL 表示默认）
 * @param[in] start_routine 线程函数
 * @param[in] arg 线程参数
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_create(osal_thread_t *thread,
						   const osal_thread_attr_t *attr,
						   void *(*start_routine)(void *), void *arg);

/**
 * @brief 等待线程结束
 *
 * @param[in] thread 线程 ID
 * @param[out] retval 线程返回值指针（NULL 表示忽略）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_join(osal_thread_t thread, void **retval);

/**
 * @brief 分离线程
 *
 * @param[in] thread 线程 ID
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_detach(osal_thread_t thread);

/**
 * @brief 比较两个线程 ID 是否相同
 *
 * @param[in] thread1 线程 ID
 * @param[in] thread2 线程 ID
 * @return true 相同，false 不同
 */
bool osal_thread_equal(osal_thread_t thread1, osal_thread_t thread2);

/**
 * @brief 获取当前线程 ID
 *
 * @return 当前线程 ID
 */
osal_thread_t osal_thread_self(void);

/**
 * @brief 退出当前线程
 *
 * @param[in] retval 返回值
 */
void osal_thread_exit(void *retval);

/**
 * @brief 取消线程
 *
 * @param[in] thread 线程 ID
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_cancel(osal_thread_t thread);

/*===========================================================================
 * 线程属性管理（可选）
 *===========================================================================*/

/**
 * @brief 初始化线程属性
 *
 * @param[in] attr 属性指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_attr_init(osal_thread_attr_t *attr);

/**
 * @brief 销毁线程属性
 *
 * @param[in] attr 属性指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_attr_destroy(osal_thread_attr_t *attr);

/**
 * @brief 设置线程栈大小
 *
 * @param[in] attr 属性指针
 * @param[in] stacksize 栈大小（字节）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_attr_set_stack_size(osal_thread_attr_t *attr,
										osal_size_t stacksize);

/**
 * @brief 获取线程栈大小
 *
 * @param[in] attr 属性指针
 * @param[out] stacksize 栈大小指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_attr_get_stack_size(const osal_thread_attr_t *attr,
										osal_size_t *stacksize);

/**
 * @brief 设置线程分离状态
 *
 * @param[in] attr 属性指针
 * @param[in] detachstate
 * 分离状态（OSAL_THREAD_CREATE_JOINABLE/OSAL_THREAD_CREATE_DETACHED）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_attr_set_detach_state(osal_thread_attr_t *attr,
										  int32_t detachstate);

/**
 * @brief 获取线程分离状态
 *
 * @param[in] attr 属性指针
 * @param[out] detachstate 分离状态指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_attr_get_detach_state(const osal_thread_attr_t *attr,
										  int32_t *detachstate);

/**
 * @brief 设置线程调度策略
 *
 * @param[in] attr 属性指针
 * @param[in] policy 调度策略（OSAL_SCHED_OTHER/OSAL_SCHED_FIFO/
 * OSAL_SCHED_RR）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_attr_set_sched_policy(osal_thread_attr_t *attr,
										  int32_t policy);

/**
 * @brief 设置线程调度参数
 *
 * @param[in] attr 属性指针
 * @param[in] param 调度参数指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_thread_attr_set_sched_param(osal_thread_attr_t *attr,
										 const osal_sched_param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_THREAD_H */
