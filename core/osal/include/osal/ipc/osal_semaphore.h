/************************************************************************
 * OSAL Semaphore API
 *
 * 面向调用方提供统一信号量接口；平台相关类型由当前 OSAL 后端映射。
 ************************************************************************/

#ifndef OSAL_SEMAPHORE_H
#define OSAL_SEMAPHORE_H

#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 信号量类型定义
 *===========================================================================*/

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
typedef sem_t osal_sem_t;
#else
/* 其他平台（RTOS 等）- 需要提供对应的类型定义 */
#error "Unsupported platform - please define semaphore types for your platform"
#endif

/*===========================================================================
 * 信号量接口
 *===========================================================================*/

/**
 * @brief 初始化信号量
 *
 * @param[in] sem 信号量指针
 * @param[in] pshared 是否进程间共享（0=否，非0=是）
 * @param[in] value 初始值
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sem_init(osal_sem_t *sem, int32_t pshared, uint32_t value);

/**
 * @brief 销毁信号量
 *
 * @param[in] sem 信号量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sem_destroy(osal_sem_t *sem);

/**
 * @brief 等待信号量（阻塞）
 *
 * @param[in] sem 信号量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sem_wait(osal_sem_t *sem);

/**
 * @brief 尝试等待信号量（非阻塞）
 *
 * @param[in] sem 信号量指针
 * @return 0 成功
 * @return -1 失败（EAGAIN 表示信号量不可用）
 */
int32_t osal_sem_try_wait(osal_sem_t *sem);

/**
 * @brief 超时等待信号量
 *
 * @param[in] sem 信号量指针
 * @param[in] timeout_ms 超时时间（毫秒），0 表示非阻塞
 * @return 0 成功
 * @return -1 失败（ETIMEDOUT 表示超时）
 */
int32_t osal_sem_timed_wait(osal_sem_t *sem, uint32_t timeout_ms);

/**
 * @brief 释放信号量
 *
 * @param[in] sem 信号量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sem_post(osal_sem_t *sem);

/**
 * @brief 获取信号量当前值
 *
 * @param[in] sem 信号量指针
 * @param[out] value 当前值
 * @return 0 成功
 * @return -1 失败
 */
int32_t osal_sem_get_value(osal_sem_t *sem, int32_t *value);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SEMAPHORE_H */
