/************************************************************************
 * 硬件配置库API实现
 *
 * 命名规范：
 * - LPF_CONFIG_*       - 通用接口
 * - LPF_CONFIG_HW_*    - 硬件配置接口
 ************************************************************************/

#include "osal.h"
#include "lpf/config/lpf_config.h"
#include "lpf_config_backend.h"
#include "lpf_config_normalizer.h"
#include "lpf_config_validator.h"
#include "generated/gen_version.h"

#define LPF_CONFIG_MAX_DEVICES 32U

static lpf_config_device_config_t g_lpf_config_devices[LPF_CONFIG_MAX_DEVICES + 1U];
static const lpf_config_backend_ops_t *g_lpf_config_backend;
static bool g_lpf_config_initialized;

const lpf_config_platform_config_t *lpf_config_get_board(void)
{
	if (!g_lpf_config_backend || !g_lpf_config_backend->active)
		return NULL;

	return g_lpf_config_backend->active();
}
EXPORT_SYMBOL_GPL(lpf_config_get_board);

const lpf_config_device_config_t *lpf_config_get(void)
{
	return g_lpf_config_initialized ? g_lpf_config_devices : NULL;
}
EXPORT_SYMBOL_GPL(lpf_config_get);

const lpf_config_platform_config_t *lpf_config_find(const char *product,
					      const char *project,
					      const char *version)
{
	const lpf_config_backend_ops_t *backend;

	backend = g_lpf_config_backend ? g_lpf_config_backend :
				      lpf_config_backend_select();
	if (!backend || !backend->find)
		return NULL;

	return backend->find(product, project, version);
}
EXPORT_SYMBOL_GPL(lpf_config_find);

int32_t lpf_config_list(const lpf_config_platform_config_t **configs, uint32_t *count)
{
	const lpf_config_backend_ops_t *backend;

	backend = g_lpf_config_backend ? g_lpf_config_backend :
				      lpf_config_backend_select();
	if (!backend || !backend->list)
		return OSAL_ERR_GENERIC;

	return backend->list(configs, count);
}
EXPORT_SYMBOL_GPL(lpf_config_list);

int32_t lpf_config_validate(const lpf_config_platform_config_t *config)
{
	const lpf_config_device_descriptor_t *descriptors;
	uint32_t descriptor_count;
	uint32_t desc_index;
	uint32_t i;
	int32_t ret;

	if (NULL == config)
		return OSAL_ERR_GENERIC;

	if (NULL == config->platform_name || NULL == config->chip_name ||
	    NULL == config->project_name || NULL == config->product_name ||
	    NULL == config->version) {
		LOG_ERROR("LPF_CONFIG", "Missing platform identity");
		return OSAL_ERR_GENERIC;
	}

	if (config->mcu_count > 0 && NULL == config->mcu_array) {
		LOG_ERROR("LPF_CONFIG", "Missing MCU config array");
		return OSAL_ERR_GENERIC;
	}

	if (config->led_count > 0 && NULL == config->led_array) {
		LOG_ERROR("LPF_CONFIG", "Missing LED config array");
		return OSAL_ERR_GENERIC;
	}

	descriptors = lpf_config_device_descriptors(&descriptor_count);
	for (desc_index = 0; desc_index < descriptor_count; desc_index++) {
		const lpf_config_device_descriptor_t *desc;
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
EXPORT_SYMBOL_GPL(lpf_config_validate);

void lpf_config_print(const lpf_config_platform_config_t *config)
{
	const lpf_config_device_descriptor_t *descriptors;
	uint32_t descriptor_count;
	uint32_t desc_index;

	if (NULL == config)
		return;

	LOG_INFO("LPF_CONFIG", "Platform: %s", config->platform_name);
	LOG_INFO("LPF_CONFIG", "Chip: %s", config->chip_name);
	LOG_INFO("LPF_CONFIG", "Project: %s", config->project_name);
	LOG_INFO("LPF_CONFIG", "Product: %s", config->product_name);
	LOG_INFO("LPF_CONFIG", "Version: %s", config->version);

	descriptors = lpf_config_device_descriptors(&descriptor_count);
	for (desc_index = 0; desc_index < descriptor_count; desc_index++) {
		const lpf_config_device_descriptor_t *desc;
		uint32_t count;
		uint32_t i;

		desc = &descriptors[desc_index];
		count = desc->count(config);
		for (i = 0; i < count; i++)
			desc->print(i, desc->entry(config, i));
	}
}
EXPORT_SYMBOL_GPL(lpf_config_print);

void lpf_config_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "LPF_CONFIG",
		 "module_version=%u.%u.%u lpf_version=%s git=%s build_time=%s build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 LPF_CONFIG_VERSION_MAJOR, LPF_CONFIG_VERSION_MINOR,
		 LPF_CONFIG_VERSION_PATCH, LPF_VERSION,
		 LPF_GIT_COMMIT, LPF_COMPILE_TIME,
		 LPF_COMPILE_BY, LPF_COMPILE_HOST,
		 LPF_COMPILER, LPF_BUILD_ARCH,
		 LPF_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(lpf_config_print_version);

int32_t lpf_config_load(void)
{
	const lpf_config_platform_config_t *platform;
	const lpf_config_backend_ops_t *backend;
	int32_t ret;

	if (g_lpf_config_initialized)
		return OSAL_SUCCESS;

	lpf_config_print_version();

	backend = lpf_config_backend_select();
	if (NULL == backend) {
		LOG_ERROR("LPF_CONFIG", "No available config backend");
		return OSAL_ERR_GENERIC;
	}

	if (!backend->load || !backend->active) {
		LOG_ERROR("LPF_CONFIG", "Invalid config backend");
		return OSAL_ERR_INVALID_PARAM;
	}

	ret = backend->load();
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("LPF_CONFIG", "Config backend load failed");
		return ret;
	}

	g_lpf_config_backend = backend;
	LOG_INFO("LPF_CONFIG", "Backend: %s",
		 g_lpf_config_backend->name ? g_lpf_config_backend->name : "unknown");

	platform = lpf_config_get_board();
	if (NULL == platform) {
		LOG_ERROR("LPF_CONFIG", "No active platform config");
		if (g_lpf_config_backend->unload)
			g_lpf_config_backend->unload();
		g_lpf_config_backend = NULL;
		return OSAL_ERR_GENERIC;
	}

	ret = lpf_config_validate(platform);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("LPF_CONFIG", "Invalid platform config");
		if (g_lpf_config_backend->unload)
			g_lpf_config_backend->unload();
		g_lpf_config_backend = NULL;
		return ret;
	}

	{
		uint32_t device_count = OSAL_ARRAY_SIZE(g_lpf_config_devices);

		ret = lpf_config_normalize_devices(platform, g_lpf_config_devices,
						   &device_count);
		if (ret == OSAL_SUCCESS)
			LOG_INFO("LPF_CONFIG", "Initialized %u device configs",
				 device_count);
	}
	if (ret != OSAL_SUCCESS) {
		if (ret == OSAL_ERR_RESOURCE_LIMIT)
			LOG_ERROR("LPF_CONFIG", "Device list capacity exceeded");
		if (g_lpf_config_backend->unload)
			g_lpf_config_backend->unload();
		g_lpf_config_backend = NULL;
		return ret;
	}

	lpf_config_print(platform);
	g_lpf_config_initialized = true;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_config_load);

void lpf_config_unload(void)
{
	osal_memset(g_lpf_config_devices, 0, sizeof(g_lpf_config_devices));
	if (g_lpf_config_backend && g_lpf_config_backend->unload)
		g_lpf_config_backend->unload();
	g_lpf_config_backend = NULL;
	g_lpf_config_initialized = false;
	LOG_INFO("LPF_CONFIG", "Deinitialized");
}
EXPORT_SYMBOL_GPL(lpf_config_unload);
