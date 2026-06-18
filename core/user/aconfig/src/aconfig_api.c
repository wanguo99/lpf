/**
 * @file aconfig_api.c
 * @brief ACONFIG 层 API 实现 - 通用配置管理框架
 * @note 提供通用的配置查询功能，不包含业务特定逻辑
 */

#include "osal.h"
#include "aconfig.h"

__attribute__((weak))
const aconfig_config_table_t g_aconfig_table = { .name = NULL,
												 .function_map = NULL,
												 .user_data = NULL };

/**
 * @brief 获取当前配置表
 */
const aconfig_config_table_t *aconfig_get_table(void)
{
	if (NULL == g_aconfig_table.name) {
		return NULL;
	}

	return &g_aconfig_table;
}
