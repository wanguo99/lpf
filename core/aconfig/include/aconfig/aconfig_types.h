/**
 * @file aconfig_types.h
 * @brief ACONFIG 核心类型定义 - 通用抽象层
 * @note 本文件提供通用的配置管理框架，不包含任何业务特定定义
 *       所有业务逻辑由产品层通过 aconfig_function_map_t 实现
 */

#ifndef ACONFIG_TYPES_H
#define ACONFIG_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 不透明的功能映射句柄
 * @note 产品层定义具体的映射实现（如 TC/TM 映射、设备引用等）
 */
typedef struct aconfig_function_map aconfig_function_map_t;

/**
 * @brief 配置表结构
 * @note 通用配置表，不包含业务特定字段
 */
typedef struct {
	const char *name;                        /* 配置表名称 */
	aconfig_function_map_t *function_map;    /* 功能映射（不透明指针） */
	void *user_data;                         /* 用户自定义数据 */
} aconfig_config_table_t;

/**
 * @brief 配置统计信息
 * @note 通用统计信息
 */
typedef struct {
	uint32_t total_functions;                /* 总功能数量 */
	uint32_t enabled_functions;              /* 启用的功能数量 */
	uint32_t disabled_functions;             /* 禁用的功能数量 */
} aconfig_statistics_t;

#endif /* ACONFIG_TYPES_H */
