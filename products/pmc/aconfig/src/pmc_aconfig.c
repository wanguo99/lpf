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

static const pmc_tc_config_t* find_tc_config(const pmc_aconfig_function_map_t *map,
						 uint32_t function_id)
{
	uint32_t i;
	uint32_t left = 0;
	uint32_t right = map->tc_count;
	bool sorted = true;

	for (i = 1; i < map->tc_count; i++) {
		if (map->tc_entries[i - 1].function_id > map->tc_entries[i].function_id) {
			sorted = false;
			break;
		}
	}

	if (!sorted) {
		for (i = 0; i < map->tc_count; i++) {
			if (map->tc_entries[i].function_id == function_id) {
				return &map->tc_entries[i].config;
			}
		}
		return NULL;
	}

	while (left < right) {
		uint32_t mid = left + (right - left) / 2;
		uint32_t mid_id = map->tc_entries[mid].function_id;

		if (mid_id == function_id) {
			return &map->tc_entries[mid].config;
		}
		if (mid_id < function_id) {
			left = mid + 1;
		} else {
			right = mid;
		}
	}

	return NULL;
}

static const pmc_tm_config_t* find_tm_config(const pmc_aconfig_function_map_t *map,
						 uint32_t function_id)
{
	uint32_t i;
	uint32_t left = 0;
	uint32_t right = map->tm_count;
	bool sorted = true;

	for (i = 1; i < map->tm_count; i++) {
		if (map->tm_entries[i - 1].function_id > map->tm_entries[i].function_id) {
			sorted = false;
			break;
		}
	}

	if (!sorted) {
		for (i = 0; i < map->tm_count; i++) {
			if (map->tm_entries[i].function_id == function_id) {
				return &map->tm_entries[i].config;
			}
		}
		return NULL;
	}

	while (left < right) {
		uint32_t mid = left + (right - left) / 2;
		uint32_t mid_id = map->tm_entries[mid].function_id;

		if (mid_id == function_id) {
			return &map->tm_entries[mid].config;
		}
		if (mid_id < function_id) {
			left = mid + 1;
		} else {
			right = mid;
		}
	}

	return NULL;
}

/**
 * @brief 查询遥控配置
 */
const pmc_tc_config_t* PMC_ACONFIG_GetTcConfig(uint32_t function_id)
{
	const pmc_aconfig_function_map_t *map;

	map = pmc_aconfig_get_map();
	if (NULL == map || NULL == map->tc_entries) {
		return NULL;
	}

	return find_tc_config(map, function_id);
}

/**
 * @brief 查询遥测配置
 */
const pmc_tm_config_t* PMC_ACONFIG_GetTmConfig(uint32_t function_id)
{
	const pmc_aconfig_function_map_t *map;

	map = pmc_aconfig_get_map();
	if (NULL == map || NULL == map->tm_entries) {
		return NULL;
	}

	return find_tm_config(map, function_id);
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
