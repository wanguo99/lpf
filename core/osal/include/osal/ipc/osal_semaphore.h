/************************************************************************
 * OSAL Semaphore API - POSIX 薄封装
 *
 * 直接暴露 POSIX sem_t 类型，提供标准 POSIX 信号量接口的薄封装
 ************************************************************************/

#ifndef OSAL_SEMAPHORE_H
#define OSAL_SEMAPHORE_H

#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * POSIX 信号量薄封装
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
int32_t OSAL_sem_init(sem_t *sem, int32_t pshared, uint32_t value);

/**
 * @brief 销毁信号量
 *
 * @param[in] sem 信号量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_sem_destroy(sem_t *sem);

/**
 * @brief 等待信号量（阻塞）
 *
 * @param[in] sem 信号量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_sem_wait(sem_t *sem);

/**
 * @brief 尝试等待信号量（非阻塞）
 *
 * @param[in] sem 信号量指针
 * @return 0 成功
 * @return -1 失败（EAGAIN 表示信号量不可用）
 */
int32_t OSAL_sem_trywait(sem_t *sem);

/**
 * @brief 超时等待信号量
 *
 * @param[in] sem 信号量指针
 * @param[in] timeout_ms 超时时间（毫秒），0 表示非阻塞
 * @return 0 成功
 * @return -1 失败（ETIMEDOUT 表示超时）
 */
int32_t OSAL_sem_timedwait(sem_t *sem, uint32_t timeout_ms);

/**
 * @brief 释放信号量
 *
 * @param[in] sem 信号量指针
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_sem_post(sem_t *sem);

/**
 * @brief 获取信号量当前值
 *
 * @param[in] sem 信号量指针
 * @param[out] value 当前值
 * @return 0 成功
 * @return -1 失败
 */
int32_t OSAL_sem_getvalue(sem_t *sem, int32_t *value);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SEMAPHORE_H */
