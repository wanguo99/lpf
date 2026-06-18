/**
 * @file aconfig.h
 * @brief ACONFIG 对外 API - 通用配置管理框架
 * @note 提供通用的配置查询接口，不包含业务特定定义
 *       业务逻辑由产品层实现（通过 aconfig_function_map_t）
 */

#ifndef ACONFIG_H
#define ACONFIG_H

#include "osal.h"

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
	const char *name; /* 配置表名称 */
	const aconfig_function_map_t *function_map; /* 功能映射（不透明指针） */
	const void *user_data; /* 用户自定义数据 */
} aconfig_config_table_t;

/**
 * @brief 配置统计信息
 * @note 通用统计信息
 */
typedef struct {
	uint32_t total_functions; /* 总功能数量 */
	uint32_t enabled_functions; /* 启用的功能数量 */
	uint32_t disabled_functions; /* 禁用的功能数量 */
} aconfig_statistics_t;

/*===========================================================================
 * 产品配置入口
 *===========================================================================*/

/**
 * @brief 产品层提供的只读应用配置表
 *
 * 产品或测试目标通过 ACONFIG_EXTRA_SRCS 编译进该符号。ACONFIG 只读取该表，
 * 不在运行期注册或释放配置。
 */
extern const aconfig_config_table_t g_aconfig_table;

/*===========================================================================
 * API 函数
 *===========================================================================*/

/**
 * @brief 获取当前配置表
 * @return 配置表指针，NULL 表示未提供有效配置
 */
const aconfig_config_table_t *aconfig_get_table(void);

#endif /* ACONFIG_H */
