/************************************************************************
 * OSAL 信号量接口
 *
 * 简单封装 POSIX 信号量，提供跨平台信号量支持
 ************************************************************************/

#ifndef OSAL_SEMAPHORE_H
#define OSAL_SEMAPHORE_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 信号量句柄类型
 *
 * 不透明句柄，内部实现为 sem_t*
 */
typedef struct osal_semaphore_s osal_semaphore_t;

/**
 * @brief 创建信号量
 *
 * @param[out] sem 信号量句柄指针
 * @param[in] initial_value 初始值（0-INT32_MAX）
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER sem 为 NULL
 * @return OSAL_ERR_INVALID_PARAM initial_value 超出范围
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_SemaphoreCreate(osal_semaphore_t **sem, uint32_t initial_value);

/**
 * @brief 销毁信号量
 *
 * @param[in] sem 信号量句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER sem 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_SemaphoreDelete(osal_semaphore_t *sem);

/**
 * @brief 等待信号量（阻塞）
 *
 * @param[in] sem 信号量句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER sem 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_SemaphoreWait(osal_semaphore_t *sem);

/**
 * @brief 等待信号量（超时）
 *
 * @param[in] sem 信号量句柄
 * @param[in] timeout_ms 超时时间（毫秒），0 表示非阻塞
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER sem 为 NULL
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_SemaphoreTimedWait(osal_semaphore_t *sem, uint32_t timeout_ms);

/**
 * @brief 释放信号量
 *
 * @param[in] sem 信号量句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER sem 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_SemaphorePost(osal_semaphore_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SEMAPHORE_H */
