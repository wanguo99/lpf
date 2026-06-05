/************************************************************************
 * OSAL 条件变量接口
 *
 * 简单封装 POSIX 条件变量，提供跨平台条件变量支持
 ************************************************************************/

#ifndef OSAL_COND_H
#define OSAL_COND_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 条件变量句柄类型
 *
 * 不透明句柄，内部实现为 pthread_cond_t*
 */
typedef struct osal_cond_s osal_cond_t;

/**
 * @brief 创建条件变量
 *
 * @param[out] cond 条件变量句柄指针
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER cond 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_CondCreate(osal_cond_t **cond);

/**
 * @brief 销毁条件变量
 *
 * @param[in] cond 条件变量句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER cond 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_CondDelete(osal_cond_t *cond);

/**
 * @brief 等待条件变量（阻塞）
 *
 * @param[in] cond 条件变量句柄
 * @param[in] mutex 互斥锁句柄（必须已锁定）
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER cond 或 mutex 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_CondWait(osal_cond_t *cond, osal_mutex_t *mutex);

/**
 * @brief 等待条件变量（超时）
 *
 * @param[in] cond 条件变量句柄
 * @param[in] mutex 互斥锁句柄（必须已锁定）
 * @param[in] timeout_ms 超时时间（毫秒）
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER cond 或 mutex 为 NULL
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_CondTimedWait(osal_cond_t *cond, osal_mutex_t *mutex, uint32_t timeout_ms);

/**
 * @brief 唤醒一个等待线程
 *
 * @param[in] cond 条件变量句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER cond 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_CondSignal(osal_cond_t *cond);

/**
 * @brief 唤醒所有等待线程
 *
 * @param[in] cond 条件变量句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER cond 为 NULL
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_CondBroadcast(osal_cond_t *cond);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_COND_H */
