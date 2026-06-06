/************************************************************************
 * OSAL Mutex API
 *
 * Mutex lock operations for thread synchronization
 ************************************************************************/

#ifndef OSAL_MUTEX_H
#define OSAL_MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 互斥锁类型定义
 *===========================================================================*/

/**
 * @brief 互斥锁句柄类型
 *
 * 不透明句柄，内部实现为 pthread_mutex_t*
 */
typedef struct osal_mutex_s osal_mutex_t;

/**
 * @brief 互斥锁类型
 */
typedef enum {
	OSAL_MUTEX_NORMAL = 0x0,      /* 普通互斥锁 */
	OSAL_MUTEX_RECURSIVE = 0x1    /* 递归锁（可重入） */
} osal_mutex_type_t;

/**
 * @brief 互斥锁属性（不透明句柄）
 */
typedef struct osal_mutex_attr_s osal_mutex_attr_t;

/*===========================================================================
 * 互斥锁属性管理 API
 *===========================================================================*/

/**
 * @brief 创建互斥锁属性对象（默认类型：OSAL_MUTEX_NORMAL）
 *
 * @param[out] attr 属性对象句柄指针
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 为 NULL
 * @return OSAL_ERR_NO_MEMORY 内存不足
 */
int32_t OSAL_MutexAttrCreate(osal_mutex_attr_t **attr);

/**
 * @brief 销毁互斥锁属性对象
 *
 * @param[in] attr 属性对象句柄
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 为 NULL
 */
int32_t OSAL_MutexAttrDestroy(osal_mutex_attr_t *attr);

/**
 * @brief 设置互斥锁类型
 *
 * @param[in] attr 属性对象句柄
 * @param[in] type 互斥锁类型
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 为 NULL
 */
int32_t OSAL_MutexAttrSetType(osal_mutex_attr_t *attr, osal_mutex_type_t type);

/**
 * @brief 获取互斥锁类型
 *
 * @param[in] attr 属性对象句柄
 * @param[out] type 互斥锁类型指针
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER attr 或 type 为 NULL
 */
int32_t OSAL_MutexAttrGetType(const osal_mutex_attr_t *attr, osal_mutex_type_t *type);

/*===========================================================================
 * 互斥锁管理 API
 *===========================================================================*/

/**
 * @brief 创建互斥锁（默认属性：普通互斥锁）
 *
 * @param[out] mutex 互斥锁句柄指针
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER mutex 为 NULL
 * @return OSAL_ERR_NO_MEMORY 内存不足
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_MutexCreate(osal_mutex_t **mutex);

/**
 * @brief 创建互斥锁（自定义属性）
 *
 * @param[out] mutex 互斥锁句柄指针
 * @param[in] attr 互斥锁属性（仅支持NORMAL/RECURSIVE类型）
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER mutex 为 NULL
 * @return OSAL_ERR_NO_MEMORY 内存不足
 * @return OSAL_ERR_GENERIC 系统调用失败
 */
int32_t OSAL_MutexCreateEx(osal_mutex_t **mutex, const osal_mutex_attr_t *attr);

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

/**
 * @brief 尝试获取互斥锁（非阻塞）
 *
 * @param[in] mutex 互斥锁句柄
 * @return OSAL_SUCCESS 成功获取锁
 * @return OSAL_ERR_INVALID_POINTER mutex 为 NULL
 * @return OSAL_ERR_BUSY 锁已被占用
 * @return OSAL_ERR_GENERIC 系统调用失败
 *
 * @note 对应pthread_mutex_trylock，不会阻塞
 */
int32_t OSAL_MutexTryLock(osal_mutex_t *mutex);

/**
 * @brief 带超时的获取互斥锁
 *
 * @param[in] mutex 互斥锁句柄
 * @param[in] timeout_ms 超时时间（毫秒）
 * @return OSAL_SUCCESS 成功获取锁
 * @return OSAL_ERR_INVALID_POINTER mutex 为 NULL
 * @return OSAL_ERR_TIMEOUT 超时
 * @return OSAL_ERR_GENERIC 系统调用失败
 *
 * @note 对应pthread_mutex_timedlock
 */
int32_t OSAL_MutexTimedLock(osal_mutex_t *mutex, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_MUTEX_H */
