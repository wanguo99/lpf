/************************************************************************
 * OSAL Mutex 私有类型定义
 *
 * 本文件包含 mutex 模块的内部结构体定义，仅供实现层使用
 * 用户代码不应 include 此文件
 ************************************************************************/

#ifndef OSAL_MUTEX_TYPES_PRIVATE_H
#define OSAL_MUTEX_TYPES_PRIVATE_H

#include "osal_mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 内部结构体定义
 *===========================================================================*/

/**
 * @brief 互斥锁属性内部结构
 */
struct osal_mutex_attr_s {
	osal_mutex_type_t type; /* 互斥锁类型 */
};

#ifdef __cplusplus
}
#endif

#endif /* OSAL_MUTEX_TYPES_PRIVATE_H */
