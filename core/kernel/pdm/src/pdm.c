// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "pdm/pdm.h"
#include "pconfig/pconfig.h"
#include "pdm_internal.h"
#include "generated/gen_version.h"

#define PDM_DRIVER_TABLE_SIZE 16U

typedef struct {
	pconfig_device_type_t type;
	const pdm_driver_ops_t *ops;
} pdm_driver_entry_t;

static pdm_driver_entry_t g_pdm_drivers[PDM_DRIVER_TABLE_SIZE];
static osal_mutex_t g_pdm_driver_lock;
static bool g_pdm_driver_lock_ready;
static bool g_pdm_builtin_drivers_ready;

static int32_t pdm_driver_registry_init(void)
{
	if (g_pdm_driver_lock_ready)
		return OSAL_SUCCESS;

	if (osal_mutex_init(&g_pdm_driver_lock, NULL) != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	g_pdm_driver_lock_ready = true;
	return OSAL_SUCCESS;
}

static void pdm_driver_registry_deinit(void)
{
	if (!g_pdm_driver_lock_ready)
		return;

	osal_memset(g_pdm_drivers, 0, sizeof(g_pdm_drivers));
	osal_mutex_destroy(&g_pdm_driver_lock);
	g_pdm_driver_lock_ready = false;
}

static const pdm_driver_ops_t *
pdm_driver_find_locked(pconfig_device_type_t type)
{
	uint32_t i;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_drivers); i++) {
		if (g_pdm_drivers[i].type == type)
			return g_pdm_drivers[i].ops;
	}

	return NULL;
}

static const pdm_builtin_driver_t *pdm_builtin_driver_first(void)
{
	return &pdm_builtin_driver_start + 1;
}

static const pdm_builtin_driver_t *pdm_builtin_driver_last(void)
{
	return &pdm_builtin_driver_end;
}

static void
pdm_builtin_drivers_exit_range(const pdm_builtin_driver_t *end)
{
	const pdm_builtin_driver_t *first = pdm_builtin_driver_first();
	const pdm_builtin_driver_t *driver = end;

	while (driver > first) {
		driver--;
		if (driver->exit)
			driver->exit();
	}
}

static int pdm_builtin_drivers_init(void)
{
	const pdm_builtin_driver_t *driver;
	int ret;

	for (driver = pdm_builtin_driver_first();
	     driver < pdm_builtin_driver_last(); driver++) {
		if (!driver->init)
			continue;

		ret = driver->init();
		if (ret) {
			LOG_ERROR("PDM", "Builtin driver %s init failed: %d",
				  driver->name ? driver->name : "unknown", ret);
			pdm_builtin_drivers_exit_range(driver);
			return ret;
		}

		LOG_INFO("PDM", "Builtin driver %s initialized",
			 driver->name ? driver->name : "unknown");
	}

	g_pdm_builtin_drivers_ready = true;
	return 0;
}

static void pdm_builtin_drivers_exit(void)
{
	if (!g_pdm_builtin_drivers_ready)
		return;

	pdm_builtin_drivers_exit_range(pdm_builtin_driver_last());
	g_pdm_builtin_drivers_ready = false;
}

static void pdm_remove_devices(void)
{
	const pdm_driver_ops_t *ops_table[PDM_DRIVER_TABLE_SIZE];
	uint32_t i;

	osal_memset(ops_table, 0, sizeof(ops_table));

	if (g_pdm_driver_lock_ready) {
		osal_mutex_lock(&g_pdm_driver_lock);
		for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_drivers); i++)
			ops_table[i] = g_pdm_drivers[i].ops;
		osal_mutex_unlock(&g_pdm_driver_lock);
	}

	for (i = 0; i < OSAL_ARRAY_SIZE(ops_table); i++) {
		if (ops_table[i] && ops_table[i]->remove_all)
			ops_table[i]->remove_all();
	}

	pconfig_unload();
}

static int pdm_probe_devices(void)
{
	const pconfig_device_config_t *device;
	int ret;

	ret = pconfig_load();
	if (ret != OSAL_SUCCESS)
		return -ret;

	device = pconfig_get();
	if (!device) {
		pconfig_unload();
		return -ENODEV;
	}

	for (; device->device_type != PCONFIG_DEVICE_TYPE_INVALID; device++) {
		const pdm_driver_ops_t *ops;

		osal_mutex_lock(&g_pdm_driver_lock);
		ops = pdm_driver_find_locked(device->device_type);
		osal_mutex_unlock(&g_pdm_driver_lock);

		if (!ops || !ops->probe) {
			LOG_ERROR("PDM", "No driver for device type=%u",
				  device->device_type);
			ret = -EOPNOTSUPP;
			goto out_error;
		}

		ret = ops->probe(device);
		if (ret != OSAL_SUCCESS) {
			ret = -ret;
			goto out_error;
		}
	}

	return 0;

out_error:
	pdm_remove_devices();
	return ret;
}

void pdm_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "PDM",
		 "module_version=%u.%u.%u middleware_version=%s git=%s build_time=%s build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 PDM_VERSION_MAJOR, PDM_VERSION_MINOR, PDM_VERSION_PATCH,
		 ES_MIDDLEWARE_VERSION, ES_MIDDLEWARE_GIT_COMMIT,
		 ES_MIDDLEWARE_COMPILE_TIME, ES_MIDDLEWARE_COMPILE_BY,
		 ES_MIDDLEWARE_COMPILE_HOST, ES_MIDDLEWARE_COMPILER,
		 ES_MIDDLEWARE_BUILD_ARCH, ES_MIDDLEWARE_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(pdm_print_version);

int32_t pdm_driver_register(pconfig_device_type_t type,
			    const pdm_driver_ops_t *ops)
{
	uint32_t i;

	if (type == PCONFIG_DEVICE_TYPE_INVALID || !ops || !ops->probe)
		return OSAL_ERR_INVALID_PARAM;

	if (pdm_driver_registry_init() != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	osal_mutex_lock(&g_pdm_driver_lock);
	if (pdm_driver_find_locked(type)) {
		osal_mutex_unlock(&g_pdm_driver_lock);
		return OSAL_ERR_ALREADY_EXISTS;
	}

	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_drivers); i++) {
		if (!g_pdm_drivers[i].ops) {
			g_pdm_drivers[i].type = type;
			g_pdm_drivers[i].ops = ops;
			osal_mutex_unlock(&g_pdm_driver_lock);
			LOG_INFO("PDM", "Registered driver type=%u", type);
			return OSAL_SUCCESS;
		}
	}
	osal_mutex_unlock(&g_pdm_driver_lock);

	return OSAL_ERR_RESOURCE_LIMIT;
}

void pdm_driver_unregister(pconfig_device_type_t type,
			   const pdm_driver_ops_t *ops)
{
	uint32_t i;

	if (!g_pdm_driver_lock_ready)
		return;

	osal_mutex_lock(&g_pdm_driver_lock);
	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_drivers); i++) {
		if (g_pdm_drivers[i].type == type &&
		    (!ops || g_pdm_drivers[i].ops == ops)) {
			g_pdm_drivers[i].type = PCONFIG_DEVICE_TYPE_INVALID;
			g_pdm_drivers[i].ops = NULL;
			break;
		}
	}
	osal_mutex_unlock(&g_pdm_driver_lock);
}

static int __init pdm_init(void)
{
	int ret;

	pdm_print_version();

	ret = pdm_driver_registry_init();
	if (ret != OSAL_SUCCESS)
		return -ret;

	ret = pdm_builtin_drivers_init();
	if (ret)
		goto out_registry_deinit;

	ret = pdm_probe_devices();
	if (ret) {
		pdm_builtin_drivers_exit();
		goto out_registry_deinit;
	}

	LOG_INFO("PDM", "loaded");
	return 0;

out_registry_deinit:
	pdm_driver_registry_deinit();
	return ret;
}

static void __exit pdm_exit(void)
{
	pdm_remove_devices();
	pdm_builtin_drivers_exit();
	pdm_driver_registry_deinit();
	LOG_INFO("PDM", "unloaded");
}

module_init(pdm_init);
module_exit(pdm_exit);

MODULE_AUTHOR("ES-Middleware");
MODULE_DESCRIPTION("ES-Middleware PDM kernel module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal pconfig hal can can_raw");
