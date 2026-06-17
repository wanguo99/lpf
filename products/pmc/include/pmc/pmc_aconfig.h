/**
 * @file pmc_aconfig.h
 * @brief PMC 产品特定的配置管理 API
 * @note 提供 PMC 特定的配置查询接口
 */

#ifndef PMC_ACONFIG_H
#define PMC_ACONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "pmc_aconfig_types.h"

/* 注意：源文件应按依赖顺序包含头文件
 * 示例：
 *   #include "osal.h"
 *   #include "aconfig.h"
 *   #include "pmc_aconfig.h"
 */

/**
 * @brief 查询遥控配置
 * @param function_id 功能 ID
 * @return 配置指针（只读），失败返回 NULL
 */
const pmc_tc_config_t* PMC_ACONFIG_GetTcConfig(uint32_t function_id);

/**
 * @brief 查询遥测配置
 * @param function_id 功能 ID
 * @return 配置指针（只读），失败返回 NULL
 */
const pmc_tm_config_t* PMC_ACONFIG_GetTmConfig(uint32_t function_id);

/**
 * @brief 检查遥控功能是否使能
 * @param function_id 功能 ID
 * @return true 使能，false 禁用
 */
bool PMC_ACONFIG_IsTcEnabled(uint32_t function_id);

/**
 * @brief 检查遥测功能是否使能
 * @param function_id 功能 ID
 * @return true 使能，false 禁用
 */
bool PMC_ACONFIG_IsTmEnabled(uint32_t function_id);

/**
 * @brief 获取失效映射
 * @param tc_function_id 遥控功能 ID
 * @param affected_ids 输出：受影响的遥测 ID 数组
 * @param max_count 数组最大容量
 * @param actual_count 输出：实际受影响的遥测数量
 * @return 0 成功，负值失败
 */
int32_t PMC_ACONFIG_GetInvalidationMap(uint32_t tc_function_id,
					uint32_t *affected_ids,
					uint32_t max_count,
					uint32_t *actual_count);

#endif /* PMC_ACONFIG_H */
