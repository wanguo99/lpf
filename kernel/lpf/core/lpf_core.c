// SPDX-License-Identifier: GPL-2.0

#include <linux/list.h>
#include <linux/module.h>

#include "lpf/lpf_core.h"
#include "lpf/lpf_soc_adapter.h"

typedef struct {
	struct list_head node;
	const lpf_driver_t *driver;
} lpf_driver_node_t;

typedef struct {
	struct list_head node;
	lpf_device_t device;
} lpf_device_node_t;

static LIST_HEAD(g_lpf_drivers);
static LIST_HEAD(g_lpf_devices);
static osal_mutex_t g_lpf_core_lock;
static bool g_lpf_core_ready;

static lpf_driver_node_t *
lpf_find_driver_node_locked(lpf_device_type_t type)
{
	lpf_driver_node_t *driver_node;

	list_for_each_entry(driver_node, &g_lpf_drivers, node) {
		if (driver_node->driver->type == type)
			return driver_node;
	}

	return NULL;
}

static lpf_driver_node_t *
lpf_find_driver_exact_locked(const lpf_driver_t *driver)
{
	lpf_driver_node_t *driver_node;

	list_for_each_entry(driver_node, &g_lpf_drivers, node) {
		if (driver_node->driver == driver)
			return driver_node;
	}

	return NULL;
}

static lpf_device_node_t *
lpf_find_device_node_locked(lpf_device_type_t type, uint32_t index)
{
	lpf_device_node_t *device_node;

	list_for_each_entry(device_node, &g_lpf_devices, node) {
		if (device_node->device.config.type == type &&
		    device_node->device.config.index == index)
			return device_node;
	}

	return NULL;
}

static lpf_device_node_t *lpf_find_device_node_by_name_locked(const char *name)
{
	lpf_device_node_t *device_node;

	if (!name)
		return NULL;

	list_for_each_entry(device_node, &g_lpf_devices, node) {
		if (0 == osal_strcmp(device_node->device.name, name))
			return device_node;
	}

	return NULL;
}

static void lpf_copy_device_info(const lpf_device_t *device,
				 lpf_device_info_t *info)
{
	const char *driver_name = NULL;

	if (!device || !info)
		return;

	osal_memset(info, 0, sizeof(*info));
	info->type = device->config.type;
	info->index = device->config.index;
	info->state = device->state;
	info->last_error = device->last_error;
	info->capabilities = device->capabilities;
	osal_strncpy(info->name, device->name, sizeof(info->name) - 1U);
	info->name[sizeof(info->name) - 1U] = '\0';

	if (device->driver)
		driver_name = device->driver->name;
	if (driver_name) {
		osal_strncpy(info->driver_name, driver_name,
			     sizeof(info->driver_name) - 1U);
		info->driver_name[sizeof(info->driver_name) - 1U] = '\0';
	}
}

static void lpf_make_device_name(lpf_device_t *device,
				 const lpf_driver_t *driver)
{
	const char *base;

	if (!device)
		return;

	base = device->config.name;
	if (base && base[0] != '\0') {
		osal_strncpy(device->name, base, sizeof(device->name));
		device->name[sizeof(device->name) - 1] = '\0';
		return;
	}

	base = driver && driver->name ? driver->name : "device";
	osal_snprintf(device->name, sizeof(device->name), "%s%u", base,
		      device->config.index);
}

int32_t lpf_core_init(void)
{
	int32_t ret;

	if (g_lpf_core_ready)
		return OSAL_SUCCESS;

	if (osal_mutex_init(&g_lpf_core_lock, NULL) != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	ret = lpf_soc_adapter_init();
	if (ret != OSAL_SUCCESS) {
		osal_mutex_destroy(&g_lpf_core_lock);
		return ret;
	}

	INIT_LIST_HEAD(&g_lpf_drivers);
	INIT_LIST_HEAD(&g_lpf_devices);
	g_lpf_core_ready = true;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_core_init);

void lpf_device_unregister_all(void)
{
	lpf_device_node_t *device_node;

	if (!g_lpf_core_ready)
		return;

	for (;;) {
		osal_mutex_lock(&g_lpf_core_lock);
		if (list_empty(&g_lpf_devices)) {
			osal_mutex_unlock(&g_lpf_core_lock);
			break;
		}

		device_node = list_last_entry(&g_lpf_devices,
					      lpf_device_node_t, node);
		list_del_init(&device_node->node);
		osal_mutex_unlock(&g_lpf_core_lock);

		if (device_node->device.driver &&
		    device_node->device.driver->remove)
			device_node->device.driver->remove(&device_node->device);
		osal_free(device_node);
	}
}
EXPORT_SYMBOL_GPL(lpf_device_unregister_all);

static void lpf_device_unregister_by_driver(const lpf_driver_t *driver)
{
	lpf_device_node_t *device_node;
	bool found;

	if (!g_lpf_core_ready || !driver)
		return;

	for (;;) {
		found = false;

		osal_mutex_lock(&g_lpf_core_lock);
		list_for_each_entry(device_node, &g_lpf_devices, node) {
			if (device_node->device.driver == driver) {
				list_del_init(&device_node->node);
				found = true;
				break;
			}
		}
		osal_mutex_unlock(&g_lpf_core_lock);

		if (!found)
			break;

		if (device_node->device.driver &&
		    device_node->device.driver->remove)
			device_node->device.driver->remove(&device_node->device);
		osal_free(device_node);
	}
}

void lpf_driver_unregister_all(void)
{
	lpf_driver_node_t *driver_node;
	lpf_driver_node_t *driver_tmp;

	if (!g_lpf_core_ready)
		return;

	lpf_device_unregister_all();

	osal_mutex_lock(&g_lpf_core_lock);
	list_for_each_entry_safe(driver_node, driver_tmp, &g_lpf_drivers,
				 node) {
		list_del(&driver_node->node);
		osal_mutex_unlock(&g_lpf_core_lock);

		if (driver_node->driver->exit)
			driver_node->driver->exit();
		osal_free(driver_node);

		osal_mutex_lock(&g_lpf_core_lock);
	}
	osal_mutex_unlock(&g_lpf_core_lock);
}
EXPORT_SYMBOL_GPL(lpf_driver_unregister_all);

void lpf_core_deinit(void)
{
	if (!g_lpf_core_ready)
		return;

	lpf_driver_unregister_all();
	lpf_soc_adapter_deinit();
	osal_mutex_destroy(&g_lpf_core_lock);
	g_lpf_core_ready = false;
}
EXPORT_SYMBOL_GPL(lpf_core_deinit);

int32_t lpf_driver_register(const lpf_driver_t *driver)
{
	lpf_driver_node_t *driver_node;
	int ret;

	if (!driver || driver->type == LPF_DEVICE_TYPE_INVALID || !driver->probe)
		return OSAL_ERR_INVALID_PARAM;

	if (lpf_core_init() != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	osal_mutex_lock(&g_lpf_core_lock);
	if (lpf_find_driver_node_locked(driver->type)) {
		osal_mutex_unlock(&g_lpf_core_lock);
		return OSAL_ERR_ALREADY_EXISTS;
	}
	osal_mutex_unlock(&g_lpf_core_lock);

	driver_node = osal_zalloc(sizeof(*driver_node));
	if (!driver_node)
		return OSAL_ERR_NO_MEMORY;

	if (driver->init) {
		ret = driver->init();
		if (ret) {
			osal_free(driver_node);
			return ret < 0 ? ret : OSAL_ERR_GENERIC;
		}
	}

	driver_node->driver = driver;

	osal_mutex_lock(&g_lpf_core_lock);
	if (lpf_find_driver_node_locked(driver->type)) {
		osal_mutex_unlock(&g_lpf_core_lock);
		if (driver->exit)
			driver->exit();
		osal_free(driver_node);
		return OSAL_ERR_ALREADY_EXISTS;
	}
	list_add_tail(&driver_node->node, &g_lpf_drivers);
	osal_mutex_unlock(&g_lpf_core_lock);

	LOG_INFO("LPF", "registered driver %s type=%u",
		 driver->name ? driver->name : "unknown", driver->type);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_driver_register);

void lpf_driver_unregister(const lpf_driver_t *driver)
{
	lpf_driver_node_t *driver_node;

	if (!g_lpf_core_ready || !driver)
		return;

	lpf_device_unregister_by_driver(driver);

	osal_mutex_lock(&g_lpf_core_lock);
	driver_node = lpf_find_driver_exact_locked(driver);
	if (!driver_node) {
		osal_mutex_unlock(&g_lpf_core_lock);
		return;
	}

	list_del(&driver_node->node);
	osal_mutex_unlock(&g_lpf_core_lock);

	if (driver_node->driver->exit)
		driver_node->driver->exit();
	osal_free(driver_node);
}
EXPORT_SYMBOL_GPL(lpf_driver_unregister);

int32_t lpf_device_register(const lpf_device_config_t *config)
{
	lpf_driver_node_t *driver_node;
	lpf_device_node_t *device_node;
	const lpf_driver_t *driver;
	int32_t ret;

	if (!config || config->type == LPF_DEVICE_TYPE_INVALID)
		return OSAL_ERR_INVALID_PARAM;

	if (lpf_core_init() != OSAL_SUCCESS)
		return OSAL_ERR_GENERIC;

	osal_mutex_lock(&g_lpf_core_lock);
	if (lpf_find_device_node_locked(config->type, config->index)) {
		osal_mutex_unlock(&g_lpf_core_lock);
		return OSAL_ERR_ALREADY_EXISTS;
	}
	driver_node = lpf_find_driver_node_locked(config->type);
	driver = driver_node ? driver_node->driver : NULL;
	osal_mutex_unlock(&g_lpf_core_lock);

	if (!driver) {
		LOG_ERROR("LPF", "no driver for device type=%u", config->type);
		return OSAL_ERR_NOT_SUPPORTED;
	}

	device_node = osal_zalloc(sizeof(*device_node));
	if (!device_node)
		return OSAL_ERR_NO_MEMORY;

	device_node->device.config = *config;
	device_node->device.driver = driver;
	device_node->device.state = LPF_DEVICE_STATE_REGISTERED;
	device_node->device.capabilities =
		config->capabilities | driver->capabilities;
	lpf_make_device_name(&device_node->device, driver);

	ret = driver->probe(&device_node->device);
	if (ret != OSAL_SUCCESS) {
		device_node->device.state = LPF_DEVICE_STATE_ERROR;
		device_node->device.last_error = ret;
		osal_free(device_node);
		return ret;
	}

	device_node->device.state = LPF_DEVICE_STATE_BOUND;

	osal_mutex_lock(&g_lpf_core_lock);
	if (lpf_find_device_node_locked(config->type, config->index)) {
		osal_mutex_unlock(&g_lpf_core_lock);
		if (driver->remove)
			driver->remove(&device_node->device);
		osal_free(device_node);
		return OSAL_ERR_ALREADY_EXISTS;
	}
	list_add_tail(&device_node->node, &g_lpf_devices);
	osal_mutex_unlock(&g_lpf_core_lock);

	LOG_INFO("LPF", "bound device %s type=%u index=%u",
		 device_node->device.name, config->type, config->index);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_device_register);

const lpf_device_t *lpf_device_find(lpf_device_type_t type, uint32_t index)
{
	lpf_device_node_t *device_node;
	const lpf_device_t *device = NULL;

	if (!g_lpf_core_ready)
		return NULL;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_find_device_node_locked(type, index);
	if (device_node)
		device = &device_node->device;
	osal_mutex_unlock(&g_lpf_core_lock);

	return device;
}
EXPORT_SYMBOL_GPL(lpf_device_find);

int32_t lpf_device_get_info(lpf_device_type_t type, uint32_t index,
			    lpf_device_info_t *info)
{
	lpf_device_node_t *device_node;

	if (!info || type == LPF_DEVICE_TYPE_INVALID)
		return OSAL_ERR_INVALID_PARAM;

	if (!g_lpf_core_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_find_device_node_locked(type, index);
	if (!device_node) {
		osal_mutex_unlock(&g_lpf_core_lock);
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	lpf_copy_device_info(&device_node->device, info);
	osal_mutex_unlock(&g_lpf_core_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_device_get_info);

int32_t lpf_device_get_info_by_name(const char *name, lpf_device_info_t *info)
{
	lpf_device_node_t *device_node;

	if (!name || !info)
		return OSAL_ERR_INVALID_PARAM;

	if (!g_lpf_core_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_find_device_node_by_name_locked(name);
	if (!device_node) {
		osal_mutex_unlock(&g_lpf_core_lock);
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	lpf_copy_device_info(&device_node->device, info);
	osal_mutex_unlock(&g_lpf_core_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_device_get_info_by_name);

int32_t lpf_device_get_info_by_capability(lpf_capability_t required,
					  uint32_t match_index,
					  lpf_device_info_t *info)
{
	lpf_device_node_t *device_node;
	uint32_t matched_count = 0;

	if (!info || required == LPF_DEVICE_CAP_NONE)
		return OSAL_ERR_INVALID_PARAM;

	if (!g_lpf_core_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_lpf_core_lock);
	list_for_each_entry(device_node, &g_lpf_devices, node) {
		if ((device_node->device.capabilities & required) != required)
			continue;

		if (matched_count++ != match_index)
			continue;

		lpf_copy_device_info(&device_node->device, info);
		osal_mutex_unlock(&g_lpf_core_lock);
		return OSAL_SUCCESS;
	}
	osal_mutex_unlock(&g_lpf_core_lock);

	return OSAL_ERR_NAME_NOT_FOUND;
}
EXPORT_SYMBOL_GPL(lpf_device_get_info_by_capability);

int32_t lpf_device_list(lpf_device_info_t *infos, uint32_t *count)
{
	lpf_device_node_t *device_node;
	uint32_t max_count;
	uint32_t actual_count = 0;
	uint32_t total_count = 0;

	if (!count)
		return OSAL_ERR_INVALID_PARAM;

	if (!g_lpf_core_ready) {
		*count = 0;
		return OSAL_ERR_INVALID_STATE;
	}

	max_count = infos ? *count : 0;

	osal_mutex_lock(&g_lpf_core_lock);
	list_for_each_entry(device_node, &g_lpf_devices, node) {
		if (infos && actual_count < max_count) {
			lpf_copy_device_info(&device_node->device,
					     &infos[actual_count]);
			actual_count++;
		}
		total_count++;
	}
	osal_mutex_unlock(&g_lpf_core_lock);

	*count = infos ? actual_count : total_count;
	return (infos && actual_count < total_count) ?
		       OSAL_ERR_RESOURCE_LIMIT :
		       OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_device_list);

static int __init lpf_core_module_init(void)
{
	int32_t ret;

	ret = lpf_core_init();
	if (ret != OSAL_SUCCESS)
		return -EINVAL;

	LOG_INFO("LPF", "core loaded");
	return 0;
}

static void __exit lpf_core_module_exit(void)
{
	lpf_core_deinit();
	LOG_INFO("LPF", "core unloaded");
}

module_init(lpf_core_module_init);
module_exit(lpf_core_module_exit);

MODULE_AUTHOR("LPF");
MODULE_DESCRIPTION("LPF core device model");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal can can_raw");
