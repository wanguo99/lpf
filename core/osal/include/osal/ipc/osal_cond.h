/************************************************************************
 * OSAL 条件变量接口
 *
 * 面向调用方提供统一条件变量接口；平台相关类型由当前 OSAL 后端映射。
 ************************************************************************/

#ifndef OSAL_COND_H
#define OSAL_COND_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 条件变量类型定义
 *===========================================================================*/

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
typedef pthread_cond_t osal_cond_t;
typedef pthread_condattr_t osal_cond_attr_t;
#else
/* 其他平台（RTOS 等）- 需要提供对应的类型定义 */
#error \
	"Unsupported platform - please define condition variable types for your platform"
#endif

/*===========================================================================
 * 条件变量接口
 *===========================================================================*/

/**
 * @brief 初始化条件变量
 *
 * @param[in] cond 条件变量指针
 * @param[in] attr 条件变量属性（NULL 表示默认）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_cond_init(osal_cond_t *cond, const osal_cond_attr_t *attr);

/**
 * @brief 销毁条件变量
 *
 * @param[in] cond 条件变量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_cond_destroy(osal_cond_t *cond);

/**
 * @brief 等待条件变量（阻塞）
 *
 * @param[in] cond 条件变量指针
 * @param[in] mutex 互斥锁指针（必须已锁定）
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_cond_wait(osal_cond_t *cond, osal_mutex_t *mutex);

/**
 * @brief 等待条件变量（超时）
 *
 * @param[in] cond 条件变量指针
 * @param[in] mutex 互斥锁指针（必须已锁定）
 * @param[in] timeout_ms 超时时间（毫秒）
 * @return 0 成功
 * @return -1 失败（ETIMEDOUT 表示超时）
 */
int32_t osal_cond_timed_wait(osal_cond_t *cond, osal_mutex_t *mutex,
							 uint32_t timeout_ms);

/**
 * @brief 唤醒一个等待线程
 *
 * @param[in] cond 条件变量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_cond_signal(osal_cond_t *cond);

/**
 * @brief 唤醒所有等待线程
 *
 * @param[in] cond 条件变量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_cond_broadcast(osal_cond_t *cond);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_COND_H */
