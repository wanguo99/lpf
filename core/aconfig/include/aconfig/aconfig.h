/**
 * @file aconfig.h
 * @brief ACONFIG 对外 API - 通用配置管理框架
 * @note 提供通用的配置注册和查询接口，不包含业务特定定义
 *       业务逻辑由产品层实现（通过 aconfig_function_map_t）
 */

#ifndef ACONFIG_H
#define ACONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* 注意：源文件应按依赖顺序包含头文件
 * 示例：
 *   #include "osal.h"
 *   #include "aconfig.h"
 *   #include "pmc_config.h"  // 产品特定配置
 */

/*===========================================================================
 * 类型定义
 *===========================================================================*/

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

/*===========================================================================
 * API 函数
 *===========================================================================*/

/**
 * @brief 初始化 ACONFIG 层
 * @return 0 成功，负值失败
 */
int32_t ACONFIG_init(void);

/**
 * @brief 清理 ACONFIG 层
 */
void ACONFIG_cleanup(void);

/**
 * @brief 注册配置表
 * @param table 配置表指针
 * @return 0 成功，负值失败
 */
int32_t ACONFIG_register_table(const aconfig_config_table_t *table);

/**
 * @brief 注销配置表
 * @return 0 成功，负值失败
 */
int32_t ACONFIG_unregister_table(void);

/**
 * @brief 获取当前配置表
 * @return 配置表指针，NULL 表示未注册
 */
const aconfig_config_table_t* ACONFIG_GetTable(void);

#endif /* ACONFIG_H */
