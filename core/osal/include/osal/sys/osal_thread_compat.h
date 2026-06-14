/************************************************************************
 * OSAL Thread 兼容层 - 临时过渡方案
 *
 * 提供旧 API 的兼容包装，使代码可以逐步迁移
 ************************************************************************/

#ifndef OSAL_THREAD_COMPAT_H
#define OSAL_THREAD_COMPAT_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 兼容类型定义
 *===========================================================================*/

/* 旧的 osal_thread_t -> 新的 pthread_t */
typedef pthread_t osal_thread_t;

/*===========================================================================
 * 兼容 API（基于新的薄封装）
 *===========================================================================*/

/**
 * @brief 创建线程（兼容旧 API）
 */
static inline int32_t OSAL_ThreadCreate(osal_thread_t *thread,
                                        void *(*start_routine)(void *),
                                        void *arg)
{
    return OSAL_pthread_create(thread, NULL, start_routine, arg);
}

/**
 * @brief 等待线程结束（兼容旧 API）
 */
static inline int32_t OSAL_ThreadJoin(osal_thread_t thread)
{
    return OSAL_pthread_join(thread, NULL);
}

/**
 * @brief 获取当前线程 ID（兼容旧 API）
 */
static inline osal_thread_t OSAL_ThreadSelf(void)
{
    return OSAL_pthread_self();
}

#ifdef __cplusplus
}
#endif

#endif /* OSAL_THREAD_COMPAT_H */
