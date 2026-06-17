/**
 * @file pmc_aconfig.c
 * @brief PMC 产品特定的配置管理实现
 * @note 提供 PMC 特定的配置查询功能
 */

#include "osal.h"
#include "aconfig.h"
#include "pmc_aconfig.h"

/**
 * @brief 获取 PMC 功能映射
 * @return PMC 功能映射指针，NULL 表示未注册
 */
static const pmc_aconfig_function_map_t* pmc_aconfig_get_map(void)
{
	const aconfig_config_table_t *table = ACONFIG_GetTable();
	if (NULL == table) {
		return NULL;
	}

	return (const pmc_aconfig_function_map_t *)table->function_map;
}

/**
 * @brief 查询遥控配置
 */
const pmc_tc_config_t* PMC_ACONFIG_GetTcConfig(uint32_t function_id)
{
	const pmc_aconfig_function_map_t *map;
	uint32_t i;

	map = pmc_aconfig_get_map();
	if (NULL == map || NULL == map->tc_entries) {
		return NULL;
	}

	/* 稀疏数组线性查找 */
	for (i = 0; i < map->tc_count; i++) {
		if (map->tc_entries[i].function_id == function_id) {
			return &map->tc_entries[i].config;
		}
	}

	return NULL;
}

/**
 * @brief 查询遥测配置
 */
const pmc_tm_config_t* PMC_ACONFIG_GetTmConfig(uint32_t function_id)
{
	const pmc_aconfig_function_map_t *map;
	uint32_t i;

	map = pmc_aconfig_get_map();
	if (NULL == map || NULL == map->tm_entries) {
		return NULL;
	}

	/* 稀疏数组线性查找 */
	for (i = 0; i < map->tm_count; i++) {
		if (map->tm_entries[i].function_id == function_id) {
			return &map->tm_entries[i].config;
		}
	}

	return NULL;
}

/**
 * @brief 检查遥控功能是否使能
 */
bool PMC_ACONFIG_IsTcEnabled(uint32_t function_id)
{
	const pmc_tc_config_t *config = PMC_ACONFIG_GetTcConfig(function_id);
	return (config != NULL) && config->enabled;
}

/**
 * @brief 检查遥测功能是否使能
 */
bool PMC_ACONFIG_IsTmEnabled(uint32_t function_id)
{
	const pmc_tm_config_t *config = PMC_ACONFIG_GetTmConfig(function_id);
	return (config != NULL) && config->enabled;
}

/**
 * @brief 获取失效映射
 */
int32_t PMC_ACONFIG_GetInvalidationMap(uint32_t tc_function_id,
					uint32_t *affected_ids,
					uint32_t max_count,
					uint32_t *actual_count)
{
	const pmc_tc_config_t *tc_config;
	uint32_t copy_count;
	uint32_t i;

	if (NULL == affected_ids || NULL == actual_count) {
		return OSAL_ERR_INVALID_POINTER;
	}

	*actual_count = 0;

	tc_config = PMC_ACONFIG_GetTcConfig(tc_function_id);
	if (NULL == tc_config) {
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	if (NULL == tc_config->invalidated_tm_ids || 0 == tc_config->invalidated_tm_count) {
		return OSAL_SUCCESS;
	}

	*actual_count = tc_config->invalidated_tm_count;

	/* 复制失效映射 */
	copy_count = (tc_config->invalidated_tm_count < max_count) ?
		     tc_config->invalidated_tm_count : max_count;

	for (i = 0; i < copy_count; i++) {
		affected_ids[i] = tc_config->invalidated_tm_ids[i];
	}

	return OSAL_SUCCESS;
}
