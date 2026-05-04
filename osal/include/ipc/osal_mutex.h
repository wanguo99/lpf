/************************************************************************
 * OSAL 互斥锁接口
 *
 * 简单封装 pthread_mutex，提供跨平台互斥锁支持
 ************************************************************************/

#ifndef OSAL_MUTEX_H
#define OSAL_MUTEX_H

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 互斥锁句柄类型
 *
 * 不透明句柄，内部实现为 pthread_mutex_t*
 */
typedef struct osal_mutex_s osal_mutex_t;

/**
 * @brief 创建互斥锁
 *
 * @param[out] mutex 互斥锁句柄指针
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER mutex 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_MutexCreate(osal_mutex_t **mutex);

/**
 * @brief 销毁互斥锁
 *
 * @param[in] mutex 互斥锁句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER mutex 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_MutexDelete(osal_mutex_t *mutex);

/**
 * @brief 获取互斥锁
 *
 * @param[in] mutex 互斥锁句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER mutex 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_MutexLock(osal_mutex_t *mutex);

/**
 * @brief 释放互斥锁
 *
 * @param[in] mutex 互斥锁句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER mutex 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_MutexUnlock(osal_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_MUTEX_H */
