// SPDX-License-Identifier: GPL-2.0

#include "osal.h"
#include "lpf_config_backend.h"

#include <linux/moduleparam.h>

#ifndef CONFIG_PROJECT_NAME
#define CONFIG_PROJECT_NAME ""
#endif

#ifndef CONFIG_PROJECT_VERSION
#define CONFIG_PROJECT_VERSION ""
#endif

static int lpf_config_static_index = -1;
static char *lpf_config_static_product;
static char *lpf_config_static_project;
static char *lpf_config_static_version;

module_param_named(config_index, lpf_config_static_index, int, 0444);
MODULE_PARM_DESC(config_index, "LPF static config index override");
module_param_named(config_product, lpf_config_static_product, charp, 0444);
MODULE_PARM_DESC(config_product, "LPF static config product selector");
module_param_named(config_project, lpf_config_static_project, charp, 0444);
MODULE_PARM_DESC(config_project, "LPF static config project selector");
module_param_named(config_version, lpf_config_static_version, charp, 0444);
MODULE_PARM_DESC(config_version, "LPF static config version selector");

static const char *lpf_config_static_selector(const char *value)
{
	return value && value[0] ? value : NULL;
}

static bool lpf_config_static_has_param_identity_selector(void)
{
	return lpf_config_static_selector(lpf_config_static_product) ||
	       lpf_config_static_selector(lpf_config_static_project) ||
	       lpf_config_static_selector(lpf_config_static_version);
}

static const char *lpf_config_static_effective_project(void)
{
	const char *project;

	project = lpf_config_static_selector(lpf_config_static_project);
	return project ? project : lpf_config_static_selector(CONFIG_PROJECT_NAME);
}

static const char *lpf_config_static_effective_version(void)
{
	const char *version;

	version = lpf_config_static_selector(lpf_config_static_version);
	return version ? version : lpf_config_static_selector(CONFIG_PROJECT_VERSION);
}

static bool lpf_config_static_has_effective_identity_selector(
	const char *product, const char *project, const char *version)
{
	return product || project || version;
}

static bool
lpf_config_static_identity_matches(const lpf_config_platform_config_t *config,
				   const char *product, const char *project,
				   const char *version)
{
	if (!config)
		return false;

	if (product) {
		if (!config->product_name ||
		    0 != osal_strcmp(config->product_name, product))
			return false;
	}

	if (project) {
		if (!config->project_name ||
		    0 != osal_strcmp(config->project_name, project))
			return false;
	}

	if (version) {
		if (!config->version ||
		    0 != osal_strcmp(config->version, version))
			return false;
	}

	return true;
}

static const lpf_config_platform_config_t *
lpf_config_static_config_at(uint32_t index)
{
	if (!g_lpf_config_platform_table.configs ||
	    index >= g_lpf_config_platform_table.count)
		return NULL;

	return g_lpf_config_platform_table.configs[index];
}

static const lpf_config_platform_config_t *
lpf_config_static_find_by_identity(const char *product, const char *project,
				   const char *version)
{
	uint32_t i;

	if (!g_lpf_config_platform_table.configs)
		return NULL;

	for (i = 0; i < g_lpf_config_platform_table.count; i++) {
		const lpf_config_platform_config_t *config;

		config = lpf_config_static_config_at(i);
		if (lpf_config_static_identity_matches(config, product, project,
						       version))
			return config;
	}

	return NULL;
}

static const lpf_config_platform_config_t *lpf_config_static_selected(void)
{
	const lpf_config_platform_config_t *config;
	const char *product = lpf_config_static_selector(lpf_config_static_product);
	const char *project = lpf_config_static_effective_project();
	const char *version = lpf_config_static_effective_version();

	if (lpf_config_static_index < -1)
		return NULL;

	if (lpf_config_static_index >= 0) {
		config = lpf_config_static_config_at(
			(uint32_t)lpf_config_static_index);
		if (!lpf_config_static_has_param_identity_selector())
			return config;

		return lpf_config_static_identity_matches(config, product,
							  project, version) ?
			       config :
			       NULL;
	}

	if (lpf_config_static_has_effective_identity_selector(product, project,
							     version))
		return lpf_config_static_find_by_identity(product, project,
							  version);

	return lpf_config_static_config_at(
		g_lpf_config_platform_table.current_index);
}

static bool lpf_config_static_available(void)
{
	return g_lpf_config_platform_table.configs &&
	       g_lpf_config_platform_table.count > 0 &&
	       lpf_config_static_selected();
}

static int32_t lpf_config_static_load(void)
{
	const lpf_config_platform_config_t *config;

	config = lpf_config_static_selected();
	if (!config) {
		LOG_ERROR("LPF_CONFIG", "No matching static config");
		return OSAL_ERR_GENERIC;
	}

	LOG_INFO("LPF_CONFIG", "Static config selected: %s/%s/%s",
		 config->product_name ? config->product_name : "unknown",
		 config->project_name ? config->project_name : "unknown",
		 config->version ? config->version : "unknown");
	return OSAL_SUCCESS;
}

static void lpf_config_static_unload(void)
{
}

static const lpf_config_platform_config_t *lpf_config_static_active(void)
{
	return lpf_config_static_selected();
}

static const lpf_config_platform_config_t *
lpf_config_static_find(const char *product, const char *project,
		    const char *version)
{
	if (NULL == product || NULL == project)
		return NULL;

	return lpf_config_static_find_by_identity(product, project, version);
}

static int32_t lpf_config_static_list(const lpf_config_platform_config_t **configs,
				   uint32_t *count)
{
	uint32_t actual_count;
	uint32_t max_count;
	uint32_t i;

	if (NULL == configs || NULL == count)
		return OSAL_ERR_GENERIC;

	if (NULL == g_lpf_config_platform_table.configs) {
		*count = 0;
		return OSAL_SUCCESS;
	}

	max_count = *count;
	actual_count = (g_lpf_config_platform_table.count < max_count) ?
			       g_lpf_config_platform_table.count :
			       max_count;

	for (i = 0; i < actual_count; i++)
		configs[i] = g_lpf_config_platform_table.configs[i];

	*count = actual_count;
	return OSAL_SUCCESS;
}

const lpf_config_backend_ops_t g_lpf_config_static_backend = {
	.name = "static",
	.available = lpf_config_static_available,
	.load = lpf_config_static_load,
	.unload = lpf_config_static_unload,
	.active = lpf_config_static_active,
	.find = lpf_config_static_find,
	.list = lpf_config_static_list,
};
