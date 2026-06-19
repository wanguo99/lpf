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
	const pdm_driver_t *driver;
} pdm_driver_entry_t;

static pdm_driver_entry_t g_pdm_drivers[PDM_DRIVER_TABLE_SIZE];
static osal_mutex_t g_pdm_driver_lock;
static bool g_pdm_driver_lock_ready;
static bool g_pdm_drivers_ready;

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

static const pdm_driver_t *
pdm_driver_find_locked(pconfig_device_type_t type)
{
	uint32_t i;

	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_drivers); i++) {
		if (g_pdm_drivers[i].type == type)
			return g_pdm_drivers[i].driver;
	}

	return NULL;
}

static const pdm_driver_t *const *pdm_driver_first(void)
{
	return &pdm_driver_start + 1;
}

static const pdm_driver_t *const *pdm_driver_last(void)
{
	return &pdm_driver_end;
}

static int32_t pdm_driver_add(const pdm_driver_t *driver)
{
	uint32_t i;

	if (!driver || driver->type == PCONFIG_DEVICE_TYPE_INVALID ||
	    !driver->probe)
		return OSAL_ERR_INVALID_PARAM;

	if (pdm_driver_registry_init() != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	osal_mutex_lock(&g_pdm_driver_lock);
	if (pdm_driver_find_locked(driver->type)) {
		osal_mutex_unlock(&g_pdm_driver_lock);
		return OSAL_ERR_ALREADY_EXISTS;
	}

	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_drivers); i++) {
		if (!g_pdm_drivers[i].driver) {
			g_pdm_drivers[i].type = driver->type;
			g_pdm_drivers[i].driver = driver;
			osal_mutex_unlock(&g_pdm_driver_lock);
			LOG_INFO("PDM", "Registered driver %s type=%u",
				 driver->name ? driver->name : "unknown",
				 driver->type);
			return OSAL_SUCCESS;
		}
	}
	osal_mutex_unlock(&g_pdm_driver_lock);

	return OSAL_ERR_RESOURCE_LIMIT;
}

static void pdm_driver_del(const pdm_driver_t *driver)
{
	uint32_t i;

	if (!g_pdm_driver_lock_ready || !driver)
		return;

	osal_mutex_lock(&g_pdm_driver_lock);
	for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_drivers); i++) {
		if (g_pdm_drivers[i].driver == driver) {
			g_pdm_drivers[i].type = PCONFIG_DEVICE_TYPE_INVALID;
			g_pdm_drivers[i].driver = NULL;
			break;
		}
	}
	osal_mutex_unlock(&g_pdm_driver_lock);
}

static void pdm_drivers_exit_range(const pdm_driver_t *const *end)
{
	const pdm_driver_t *const *first = pdm_driver_first();
	const pdm_driver_t *const *entry = end;

	while (entry > first) {
		const pdm_driver_t *driver;

		entry--;
		driver = *entry;
		if (!driver)
			continue;

		pdm_driver_del(driver);
		if (driver->exit)
			driver->exit();
	}
}

static int pdm_drivers_init(void)
{
	const pdm_driver_t *const *entry;
	int ret;

	for (entry = pdm_driver_first(); entry < pdm_driver_last(); entry++) {
		const pdm_driver_t *driver = *entry;

		if (!driver)
			continue;

		if (driver->init) {
			ret = driver->init();
			if (ret) {
				LOG_ERROR("PDM", "Driver %s init failed: %d",
					  driver->name ? driver->name :
							 "unknown",
					  ret);
				pdm_drivers_exit_range(entry);
				return ret;
			}
		}

		ret = pdm_driver_add(driver);
		if (ret) {
			LOG_ERROR("PDM", "Driver %s register failed: %d",
				  driver->name ? driver->name : "unknown",
				  ret);
			if (driver->exit)
				driver->exit();
			pdm_drivers_exit_range(entry);
			return -ret;
		}
	}

	g_pdm_drivers_ready = true;
	return 0;
}

static void pdm_drivers_exit(void)
{
	if (!g_pdm_drivers_ready)
		return;

	pdm_drivers_exit_range(pdm_driver_last());
	g_pdm_drivers_ready = false;
}

static void pdm_remove_devices(void)
{
	const pdm_driver_t *driver_table[PDM_DRIVER_TABLE_SIZE];
	uint32_t i;

	osal_memset(driver_table, 0, sizeof(driver_table));

	if (g_pdm_driver_lock_ready) {
		osal_mutex_lock(&g_pdm_driver_lock);
		for (i = 0; i < OSAL_ARRAY_SIZE(g_pdm_drivers); i++)
			driver_table[i] = g_pdm_drivers[i].driver;
		osal_mutex_unlock(&g_pdm_driver_lock);
	}

	for (i = 0; i < OSAL_ARRAY_SIZE(driver_table); i++) {
		if (driver_table[i] && driver_table[i]->remove_all)
			driver_table[i]->remove_all();
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
		const pdm_driver_t *driver;

		osal_mutex_lock(&g_pdm_driver_lock);
		driver = pdm_driver_find_locked(device->device_type);
		osal_mutex_unlock(&g_pdm_driver_lock);

		if (!driver || !driver->probe) {
			LOG_ERROR("PDM", "No driver for device type=%u",
				  device->device_type);
			ret = -EOPNOTSUPP;
			goto out_error;
		}

		ret = driver->probe(device);
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

static int __init pdm_init(void)
{
	int ret;

	pdm_print_version();

	ret = pdm_driver_registry_init();
	if (ret != OSAL_SUCCESS)
		return -ret;

	ret = pdm_drivers_init();
	if (ret)
		goto out_registry_deinit;

	ret = pdm_probe_devices();
	if (ret) {
		pdm_drivers_exit();
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
	pdm_drivers_exit();
	pdm_driver_registry_deinit();
	LOG_INFO("PDM", "unloaded");
}

module_init(pdm_init);
module_exit(pdm_exit);

MODULE_AUTHOR("ES-Middleware");
MODULE_DESCRIPTION("ES-Middleware PDM kernel module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal pconfig hal can can_raw");
