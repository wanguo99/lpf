/**
 * @file aconfig.h
 * @brief ACONFIG 对外 API - 核心配置管理接口
 * @note 本文件为 ACONFIG 模块的对外 API，提供配置注册和查询功能
 *
 * ACONFIG 提供稳定的应用配置层 API，将业务功能映射到硬件设备。
 * 所有 include 都是无条件编译的核心依赖，因为：
 * - ACONFIG 必须提供一致的 API 表面，无论底层硬件配置如何
 * - 条件编译发生在实现层（aconfig.c）和底层（HAL/PDL/PRL）
 * - 这确保了 API 的稳定性和向后兼容性
 */

#ifndef ACONFIG_H
#define ACONFIG_H

/* Core dependencies - always required */
#include "osal.h"              /* OSAL 基础类型和返回值 */
#include "aconfig_types.h"     /* ACONFIG 核心数据结构 */
#include "aconfig_tc.h"        /* 遥控功能 ID 枚举 */
#include "aconfig_tm.h"        /* 遥测功能 ID 枚举 */

/**
 * @brief ACONFIG 配置统计信息
 */
typedef struct {
	uint32_t tc_enabled_count;	/* 使能的遥控功能数量 */
	uint32_t tc_disabled_count;	/* 禁用的遥控功能数量 */
	uint32_t tm_enabled_count;	/* 使能的遥测功能数量 */
	uint32_t tm_disabled_count;	/* 禁用的遥测功能数量 */
	uint32_t invalidation_map_count; /* 失效映射数量 */
} aconfig_statistics_t;

/**
 * @brief 初始化 ACONFIG 层
 * @return OSAL_SUCCESS 成功，OSAL_ERR_* 失败
 */
int32_t ACONFIG_Init(void);

/**
 * @brief 注册配置表
 * @param table 配置表指针
 * @return OSAL_SUCCESS 成功，OSAL_ERR_* 失败
 * @note 通常在项目初始化时调用，注册项目特定的配置
 */
int32_t ACONFIG_RegisterTable(const aconfig_config_table_t *table);

/**
 * @brief 查询遥控配置（O(1)）
 * @param function_id 功能 ID
 * @return 配置指针（只读），失败返回 NULL
 */
const aconfig_tc_config_t* ACONFIG_GetTcConfig(uint32_t function_id);

/**
 * @brief 查询遥测配置（O(1)）
 * @param function_id 功能 ID
 * @return 配置指针（只读），失败返回 NULL
 */
const aconfig_tm_config_t* ACONFIG_GetTmConfig(uint32_t function_id);

/**
 * @brief 检查遥控功能是否使能
 * @param function_id 功能 ID
 * @return true 使能，false 禁用
 */
bool ACONFIG_IsTcEnabled(uint32_t function_id);

/**
 * @brief 检查遥测功能是否使能
 * @param function_id 功能 ID
 * @return true 使能，false 禁用
 */
bool ACONFIG_IsTmEnabled(uint32_t function_id);

/**
 * @brief 获取失效映射
 * @param source_tm_id 源遥测 ID
 * @param affected_ids 输出：受影响的遥测 ID 数组（调用者分配）
 * @param max_count 数组最大容量
 * @param actual_count 输出：实际受影响的遥测数量
 * @return OSAL_SUCCESS 成功，OSAL_ERR_* 失败
 */
int32_t ACONFIG_GetInvalidationMap(uint32_t source_tm_id,
				uint32_t *affected_ids,
				uint32_t max_count,
				uint32_t *actual_count);

/**
 * @brief 获取配置统计信息
 * @param stats 输出：统计信息
 * @return OSAL_SUCCESS 成功，OSAL_ERR_* 失败
 */
int32_t ACONFIG_GetStatistics(aconfig_statistics_t *stats);

/**
 * @brief 根据HWID查找配置表
 * @param hwid 硬件ID
 * @return 配置表指针，失败返回NULL
 * @note 匹配规则：
 *       - 如果配置表的 hwid_count == 0 或 hwid_list == NULL，表示支持所有HWID
 *       - 否则，hwid 必须在 hwid_list 中才匹配
 */
const aconfig_config_table_t* ACONFIG_FindTableByHWID(pdl_hwid_t hwid);

/**
 * @brief 根据HWID自动加载配置表
 * @return OSAL_SUCCESS 成功，OSAL_ERR_* 失败
 * @note 从PDL_MISC读取HWID，然后查找并注册匹配的配置表
 */
int32_t ACONFIG_LoadByHWID(void);

/**
 * @brief 打印配置信息（调试用）
 */
void ACONFIG_PrintConfig(void);

#endif /* ACONFIG_H */
