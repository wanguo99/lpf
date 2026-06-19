/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - PCONFIG_*       - 通用接口
 * - PCONFIG_HW_*    - 硬件配置接口
 ************************************************************************/

#include "osal.h"
#include "pconfig/pconfig.h"
#include "generated/gen_version.h"

#include <linux/module.h>

#define PCONFIG_MAX_DEVICES 32U

__attribute__((weak))
const pconfig_platform_table_t g_pconfig_platform_table = { .configs = NULL,
															.count = 0,
															.current_index =
																0 };

static pconfig_device_config_t g_pconfig_devices[PCONFIG_MAX_DEVICES + 1U];
static bool g_pconfig_initialized;

void pconfig_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "PCONFIG",
		 "module_version=%u.%u.%u middleware_version=%s git=%s build_time=%s build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 PCONFIG_VERSION_MAJOR, PCONFIG_VERSION_MINOR,
		 PCONFIG_VERSION_PATCH, ES_MIDDLEWARE_VERSION,
		 ES_MIDDLEWARE_GIT_COMMIT, ES_MIDDLEWARE_COMPILE_TIME,
		 ES_MIDDLEWARE_COMPILE_BY, ES_MIDDLEWARE_COMPILE_HOST,
		 ES_MIDDLEWARE_COMPILER, ES_MIDDLEWARE_BUILD_ARCH,
		 ES_MIDDLEWARE_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(pconfig_print_version);

const pconfig_device_config_t *pconfig_get(void)
{
	return g_pconfig_initialized ? g_pconfig_devices : NULL;
}
EXPORT_SYMBOL_GPL(pconfig_get);

int32_t pconfig_init(void)
{
	const pconfig_platform_config_t *platform;
	uint32_t out_index = 0;
	uint32_t i;
	int32_t ret;

	pconfig_print_version();

	platform = pconfig_get_board();
	if (NULL == platform) {
		LOG_ERROR("PCONFIG", "No active platform config");
		return OSAL_ERR_GENERIC;
	}

	ret = pconfig_validate(platform);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("PCONFIG", "Invalid platform config");
		return ret;
	}

	osal_memset(g_pconfig_devices, 0, sizeof(g_pconfig_devices));

	for (i = 0; i < platform->mcu_count; i++) {
		const pconfig_mcu_entry_t *entry;

		if (out_index >= PCONFIG_MAX_DEVICES) {
			LOG_ERROR("PCONFIG", "Device list capacity exceeded");
			return OSAL_ERR_RESOURCE_LIMIT;
		}

		entry = pconfig_hw_get_mcu(platform, i);
		if (NULL == entry || !entry->enabled) {
			continue;
		}

		g_pconfig_devices[out_index].device_type =
			PCONFIG_DEVICE_TYPE_MCU;
		g_pconfig_devices[out_index].index = i;
		g_pconfig_devices[out_index].entry = entry;
		out_index++;
	}

	g_pconfig_devices[out_index].device_type = PCONFIG_DEVICE_TYPE_INVALID;
	g_pconfig_initialized = true;

	LOG_INFO("PCONFIG", "Initialized %u device configs", out_index);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(pconfig_init);

void pconfig_deinit(void)
{
	osal_memset(g_pconfig_devices, 0, sizeof(g_pconfig_devices));
	g_pconfig_initialized = false;
	LOG_INFO("PCONFIG", "Deinitialized");
}
EXPORT_SYMBOL_GPL(pconfig_deinit);

/*===========================================================================
 * 平台配置查询
 *===========================================================================*/

const pconfig_platform_config_t *pconfig_get_board(void)
{
	if (NULL == g_pconfig_platform_table.configs ||
		0u == g_pconfig_platform_table.count ||
		g_pconfig_platform_table.current_index >=
			g_pconfig_platform_table.count) {
		return NULL;
	}

	return g_pconfig_platform_table
		.configs[g_pconfig_platform_table.current_index];
}

const pconfig_platform_config_t *pconfig_find(const char *platform,
											  const char *product,
											  const char *version
											  __attribute__((unused)))
{
	uint32_t i;
	const pconfig_platform_config_t *config;

	if (NULL == platform || NULL == product ||
		NULL == g_pconfig_platform_table.configs) {
		return NULL;
	}

	for (i = 0; i < g_pconfig_platform_table.count; i++) {
		config = g_pconfig_platform_table.configs[i];
		if (NULL == config) {
			continue;
		}

		if (NULL == config->platform_name || NULL == config->product_name) {
			continue;
		}

		if (0 != osal_strcmp(config->platform_name, platform)) {
			continue;
		}

		if (0 != osal_strcmp(config->product_name, product)) {
			continue;
		}

		return config;
	}

	return NULL;
}

int32_t pconfig_list(const pconfig_platform_config_t **configs, uint32_t *count)
{
	uint32_t max_count;
	uint32_t actual_count;
	uint32_t i;

	if (NULL == configs || NULL == count) {
		return OSAL_ERR_GENERIC;
	}

	if (NULL == g_pconfig_platform_table.configs) {
		*count = 0;
		return OSAL_SUCCESS;
	}

	max_count = *count;
	actual_count = (g_pconfig_platform_table.count < max_count) ?
					   g_pconfig_platform_table.count :
					   max_count;

	for (i = 0; i < actual_count; i++) {
		configs[i] = g_pconfig_platform_table.configs[i];
	}

	*count = actual_count;

	return OSAL_SUCCESS;
}

/*===========================================================================
 * 硬件外设配置查询接口（PCONFIG_HW_*）
 *
 * 说明：所有 Get 函数已改为 inline 函数，在头文件中实现
 *===========================================================================*/

/*===========================================================================
 * 配置验证
 *===========================================================================*/

int32_t pconfig_validate(const pconfig_platform_config_t *config)
{
	if (NULL == config) {
		return OSAL_ERR_GENERIC;
	}

	if (NULL == config->platform_name || NULL == config->product_name) {
		LOG_ERROR("PCONFIG", "Missing platform or product name");
		return OSAL_ERR_GENERIC;
	}

	return OSAL_SUCCESS;
}

void pconfig_print(const pconfig_platform_config_t *config)
{
	uint32_t i;

	if (NULL == config) {
		return;
	}

	LOG_INFO("PCONFIG", "Platform: %s", config->platform_name);
	LOG_INFO("PCONFIG", "Product: %s", config->product_name);

	/* 打印MCU配置 */
	if (config->mcu_array) {
		for (i = 0; i < config->mcu_count; i++) {
			LOG_INFO("PCONFIG", "  MCU[%u]: %s", i,
					 config->mcu_array[i].description ?
						 config->mcu_array[i].description :
						 "N/A");
		}
	}
}
