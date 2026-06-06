/************************************************************************
 * OSAL Thread API
 *
 * Thread creation, management, and synchronization primitives
 ************************************************************************/

#ifndef OSAL_THREAD_H
#define OSAL_THREAD_H

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
 * @brief 线程属性（不透明句柄）
 */
typedef struct osal_thread_attr_s osal_thread_attr_t;

/*===========================================================================
 * 线程属性管理 API
 *===========================================================================*/

/**
 * @brief 创建线程属性对象（默认值）
 *
 * @param[out] attr 属性对象句柄指针
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 为 NULL
 * @return OSAL_ERR_NO_MEMORY 内存不足
 */
int32_t OSAL_ThreadAttrCreate(osal_thread_attr_t **attr);

/**
 * @brief 销毁线程属性对象
 *
 * @param[in] attr 属性对象句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 为 NULL
 */
int32_t OSAL_ThreadAttrDestroy(osal_thread_attr_t *attr);

/**
 * @brief 设置线程栈大小
 *
 * @param[in] attr 属性对象句柄
 * @param[in] stack_size 栈大小（字节），0 表示使用默认值
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 为 NULL
 */
int32_t OSAL_ThreadAttrSetStackSize(osal_thread_attr_t *attr, size_t stack_size);

/**
 * @brief 设置线程分离状态
 *
 * @param[in] attr 属性对象句柄
 * @param[in] detached true=分离线程，false=可连接线程
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 为 NULL
 */
int32_t OSAL_ThreadAttrSetDetached(osal_thread_attr_t *attr, bool detached);

/**
 * @brief 设置线程调度策略和优先级
 *
 * @param[in] attr 属性对象句柄
 * @param[in] policy 调度策略（SCHED_OTHER/FIFO/RR）
 * @param[in] priority 调度优先级（1-99）
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 为 NULL
 */
int32_t OSAL_ThreadAttrSetSched(osal_thread_attr_t *attr, int32_t policy, int32_t priority);

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

#endif /* OSAL_THREAD_H */
