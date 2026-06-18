/************************************************************************
 * OSAL Thread 私有类型定义
 *
 * 本文件包含 thread 模块的内部结构体定义，仅供实现层使用
 ************************************************************************/

#ifndef OSAL_THREAD_TYPES_PRIVATE_H
#define OSAL_THREAD_TYPES_PRIVATE_H

#include "osal_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 线程属性内部结构
 */
struct osal_thread_attr_s {
	size_t stack_size; /* 栈大小，0表示默认 */
	bool detached; /* 是否分离 */
	int32_t inherit_sched; /* 是否继承调度属性（0=不继承，1=继承） */
	int32_t sched_policy; /* 调度策略（SCHED_OTHER/FIFO/RR） */
	int32_t sched_priority; /* 调度优先级（1-99） */
};

#ifdef __cplusplus
}
#endif

#endif /* OSAL_THREAD_TYPES_PRIVATE_H */
