/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - PCONFIG_*       - 通用接口
 * - PCONFIG_HW_*    - 硬件配置接口
 ************************************************************************/

#include "osal.h"
#include "pconfig/pconfig.h"
#include "pconfig_backend.h"
#include "pconfig_validator.h"
#include "generated/gen_version.h"

#define PCONFIG_MAX_DEVICES 32U

static pconfig_device_config_t g_pconfig_devices[PCONFIG_MAX_DEVICES + 1U];
static const pconfig_backend_ops_t *g_pconfig_backend;
static bool g_pconfig_initialized;

static int32_t
pconfig_build_device_list(const pconfig_platform_config_t *platform)
{
	const pconfig_device_descriptor_t *descriptors;
	uint32_t descriptor_count;
	uint32_t out_index = 0;
	uint32_t desc_index;

	osal_memset(g_pconfig_devices, 0, sizeof(g_pconfig_devices));

	descriptors = pconfig_device_descriptors(&descriptor_count);
	for (desc_index = 0; desc_index < descriptor_count; desc_index++) {
		const pconfig_device_descriptor_t *desc;
		uint32_t count;
		uint32_t i;

		desc = &descriptors[desc_index];
		count = desc->count(platform);
		for (i = 0; i < count; i++) {
			const void *entry;

			if (out_index >= PCONFIG_MAX_DEVICES) {
				LOG_ERROR("PCONFIG",
					  "Device list capacity exceeded");
				return OSAL_ERR_RESOURCE_LIMIT;
			}

			entry = desc->entry(platform, i);
			if (!desc->enabled(entry))
				continue;

			g_pconfig_devices[out_index].device_type = desc->type;
			g_pconfig_devices[out_index].index = i;
			g_pconfig_devices[out_index].entry = entry;
			out_index++;
		}
	}

	g_pconfig_devices[out_index].device_type = PCONFIG_DEVICE_TYPE_INVALID;
	LOG_INFO("PCONFIG", "Initialized %u device configs", out_index);
	return OSAL_SUCCESS;
}

const pconfig_platform_config_t *pconfig_get_board(void)
{
	if (!g_pconfig_backend || !g_pconfig_backend->active)
		return NULL;

	return g_pconfig_backend->active();
}
EXPORT_SYMBOL_GPL(pconfig_get_board);

const pconfig_device_config_t *pconfig_get(void)
{
	return g_pconfig_initialized ? g_pconfig_devices : NULL;
}
EXPORT_SYMBOL_GPL(pconfig_get);

const pconfig_platform_config_t *pconfig_find(const char *product,
					      const char *project,
					      const char *version)
{
	const pconfig_backend_ops_t *backend;

	backend = g_pconfig_backend ? g_pconfig_backend :
				      pconfig_backend_select();
	if (!backend || !backend->find)
		return NULL;

	return backend->find(product, project, version);
}
EXPORT_SYMBOL_GPL(pconfig_find);

int32_t pconfig_list(const pconfig_platform_config_t **configs, uint32_t *count)
{
	const pconfig_backend_ops_t *backend;

	backend = g_pconfig_backend ? g_pconfig_backend :
				      pconfig_backend_select();
	if (!backend || !backend->list)
		return OSAL_ERR_GENERIC;

	return backend->list(configs, count);
}
EXPORT_SYMBOL_GPL(pconfig_list);

int32_t pconfig_validate(const pconfig_platform_config_t *config)
{
	const pconfig_device_descriptor_t *descriptors;
	uint32_t descriptor_count;
	uint32_t desc_index;
	uint32_t i;
	int32_t ret;

	if (NULL == config)
		return OSAL_ERR_GENERIC;

	if (NULL == config->platform_name || NULL == config->chip_name ||
	    NULL == config->project_name || NULL == config->product_name ||
	    NULL == config->version) {
		LOG_ERROR("PCONFIG", "Missing platform identity");
		return OSAL_ERR_GENERIC;
	}

	if (config->mcu_count > 0 && NULL == config->mcu_array) {
		LOG_ERROR("PCONFIG", "Missing MCU config array");
		return OSAL_ERR_GENERIC;
	}

	if (config->led_count > 0 && NULL == config->led_array) {
		LOG_ERROR("PCONFIG", "Missing LED config array");
		return OSAL_ERR_GENERIC;
	}

	descriptors = pconfig_device_descriptors(&descriptor_count);
	for (desc_index = 0; desc_index < descriptor_count; desc_index++) {
		const pconfig_device_descriptor_t *desc;
		uint32_t count;

		desc = &descriptors[desc_index];
		count = desc->count(config);
		for (i = 0; i < count; i++) {
			ret = desc->validate(i, desc->entry(config, i));
			if (ret != OSAL_SUCCESS)
				return ret;
		}
	}

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(pconfig_validate);

void pconfig_print(const pconfig_platform_config_t *config)
{
	const pconfig_device_descriptor_t *descriptors;
	uint32_t descriptor_count;
	uint32_t desc_index;

	if (NULL == config)
		return;

	LOG_INFO("PCONFIG", "Platform: %s", config->platform_name);
	LOG_INFO("PCONFIG", "Chip: %s", config->chip_name);
	LOG_INFO("PCONFIG", "Project: %s", config->project_name);
	LOG_INFO("PCONFIG", "Product: %s", config->product_name);
	LOG_INFO("PCONFIG", "Version: %s", config->version);

	descriptors = pconfig_device_descriptors(&descriptor_count);
	for (desc_index = 0; desc_index < descriptor_count; desc_index++) {
		const pconfig_device_descriptor_t *desc;
		uint32_t count;
		uint32_t i;

		desc = &descriptors[desc_index];
		count = desc->count(config);
		for (i = 0; i < count; i++)
			desc->print(i, desc->entry(config, i));
	}
}
EXPORT_SYMBOL_GPL(pconfig_print);

void pconfig_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "PCONFIG",
		 "module_version=%u.%u.%u lpf_version=%s git=%s build_time=%s build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 PCONFIG_VERSION_MAJOR, PCONFIG_VERSION_MINOR,
		 PCONFIG_VERSION_PATCH, LPF_VERSION,
		 LPF_GIT_COMMIT, LPF_COMPILE_TIME,
		 LPF_COMPILE_BY, LPF_COMPILE_HOST,
		 LPF_COMPILER, LPF_BUILD_ARCH,
		 LPF_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(pconfig_print_version);

int32_t pconfig_load(void)
{
	const pconfig_platform_config_t *platform;
	const pconfig_backend_ops_t *backend;
	int32_t ret;

	if (g_pconfig_initialized)
		return OSAL_SUCCESS;

	pconfig_print_version();

	backend = pconfig_backend_select();
	if (NULL == backend) {
		LOG_ERROR("PCONFIG", "No available config backend");
		return OSAL_ERR_GENERIC;
	}

	if (!backend->load || !backend->active) {
		LOG_ERROR("PCONFIG", "Invalid config backend");
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = backend->load();
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("PCONFIG", "Config backend load failed");
		return ret;
	}

	g_pconfig_backend = backend;
	LOG_INFO("PCONFIG", "Backend: %s",
		 g_pconfig_backend->name ? g_pconfig_backend->name : "unknown");

	platform = pconfig_get_board();
	if (NULL == platform) {
		LOG_ERROR("PCONFIG", "No active platform config");
		if (g_pconfig_backend->unload)
			g_pconfig_backend->unload();
		g_pconfig_backend = NULL;
		return OSAL_ERR_GENERIC;
	}

	ret = pconfig_validate(platform);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("PCONFIG", "Invalid platform config");
		if (g_pconfig_backend->unload)
			g_pconfig_backend->unload();
		g_pconfig_backend = NULL;
		return ret;
	}

	ret = pconfig_build_device_list(platform);
	if (ret != OSAL_SUCCESS) {
		if (g_pconfig_backend->unload)
			g_pconfig_backend->unload();
		g_pconfig_backend = NULL;
		return ret;
	}

	pconfig_print(platform);
	g_pconfig_initialized = true;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(pconfig_load);

void pconfig_unload(void)
{
	osal_memset(g_pconfig_devices, 0, sizeof(g_pconfig_devices));
	if (g_pconfig_backend && g_pconfig_backend->unload)
		g_pconfig_backend->unload();
	g_pconfig_backend = NULL;
	g_pconfig_initialized = false;
	LOG_INFO("PCONFIG", "Deinitialized");
}
EXPORT_SYMBOL_GPL(pconfig_unload);
