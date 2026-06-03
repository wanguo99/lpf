/**
 * @file aconfig_internal.h
 * @brief ACONFIG 内部实现 - 模块内部数据结构和辅助函数
 * @note 本文件仅供 ACONFIG 模块内部使用，外部不应包含此文件
 */

#ifndef ACONFIG_INTERNAL_H
#define ACONFIG_INTERNAL_H

#include "osal_types.h"
#include "../api/aconfig_types.h"

/**
 * @brief ACONFIG 模块内部全局状态
 * @note 使用全局变量管理配置表和锁
 */
typedef struct {
	const aconfig_config_table_t *table;	/* 配置表指针 */
	osal_rwlock_t *rwlock;			/* 读写锁 */
} aconfig_internal_state_t;

#endif /* ACONFIG_INTERNAL_H */
