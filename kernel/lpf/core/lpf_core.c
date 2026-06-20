// SPDX-License-Identifier: GPL-2.0

#include <linux/list.h>
#include <linux/module.h>
#include <linux/notifier.h>

#include "lpf/lpf_core.h"
#include "lpf/soc/lpf_soc_adapter.h"
#include "lpf_ctl_internal.h"

typedef struct {
	struct list_head node;
	const lpf_driver_t *driver;
} lpf_driver_node_t;

typedef struct {
	struct list_head node;
	lpf_device_t device;
	osal_atomic_uint32_t ref_count;
	bool removing;
} lpf_device_node_t;

struct lpf_device_handle {
	lpf_device_node_t *node;
};

typedef struct {
	struct list_head node;
	struct notifier_block notifier;
	lpf_device_event_callback_t callback;
	void *user_data;
} lpf_device_event_subscriber_t;

static LIST_HEAD(g_lpf_drivers);
static LIST_HEAD(g_lpf_devices);
static LIST_HEAD(g_lpf_event_subscribers);
static BLOCKING_NOTIFIER_HEAD(g_lpf_device_notifier);
static osal_mutex_t g_lpf_core_lock;
static osal_mutex_t g_lpf_event_lock;
static osal_cond_t g_lpf_device_ref_cond;
static bool g_lpf_core_ready;

static void lpf_copy_device_info(const lpf_device_t *device,
				 lpf_device_info_t *info);

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

static lpf_device_node_t *
lpf_device_node_from_handle_locked(const lpf_device_handle_t *handle)
{
	if (!handle)
		return NULL;

	return handle->node;
}

static void lpf_make_device_event(const lpf_device_t *device,
				  lpf_device_event_type_t type,
				  int32_t status,
				  lpf_device_event_t *event)
{
	if (!event)
		return;

	osal_memset(event, 0, sizeof(*event));
	event->type = type;
	event->status = status;
	lpf_copy_device_info(device, &event->device);
}

static void lpf_emit_device_event(const lpf_device_event_t *event)
{
	if (!event)
		return;

	blocking_notifier_call_chain(&g_lpf_device_notifier, event->type,
				     (void *)event);
}

static void lpf_emit_device_event_for_device(const lpf_device_t *device,
					     lpf_device_event_type_t type,
					     int32_t status)
{
	lpf_device_event_t event;

	lpf_make_device_event(device, type, status, &event);
	lpf_emit_device_event(&event);
}

static int lpf_device_event_notifier_call(struct notifier_block *notifier,
					  unsigned long action, void *data)
{
	lpf_device_event_subscriber_t *subscriber;

	(void)action;

	if (!data)
		return NOTIFY_DONE;

	subscriber = container_of(notifier, lpf_device_event_subscriber_t,
				  notifier);
	if (subscriber->callback)
		subscriber->callback((const lpf_device_event_t *)data,
				     subscriber->user_data);

	return NOTIFY_OK;
}

static lpf_device_event_subscriber_t *
lpf_find_event_subscriber_locked(lpf_device_event_callback_t callback,
				 void *user_data)
{
	lpf_device_event_subscriber_t *subscriber;

	list_for_each_entry(subscriber, &g_lpf_event_subscribers, node) {
		if (subscriber->callback == callback &&
		    subscriber->user_data == user_data)
			return subscriber;
	}

	return NULL;
}

static void lpf_device_event_unsubscribe_all(void)
{
	lpf_device_event_subscriber_t *subscriber;

	if (!g_lpf_core_ready)
		return;

	for (;;) {
		osal_mutex_lock(&g_lpf_event_lock);
		if (list_empty(&g_lpf_event_subscribers)) {
			osal_mutex_unlock(&g_lpf_event_lock);
			break;
		}

		subscriber = list_first_entry(&g_lpf_event_subscribers,
					      lpf_device_event_subscriber_t,
					      node);
		list_del_init(&subscriber->node);
		osal_mutex_unlock(&g_lpf_event_lock);

		blocking_notifier_chain_unregister(&g_lpf_device_notifier,
						   &subscriber->notifier);
		osal_free(subscriber);
	}
}

static lpf_device_handle_t *
lpf_device_handle_create_locked(lpf_device_node_t *device_node)
{
	lpf_device_handle_t *handle;

	if (!device_node || device_node->removing)
		return NULL;

	handle = osal_zalloc(sizeof(*handle));
	if (!handle)
		return NULL;

	osal_atomic_inc(&device_node->ref_count);
	handle->node = device_node;
	return handle;
}

static void lpf_device_wait_idle_locked(lpf_device_node_t *device_node)
{
	if (!device_node)
		return;

	while (osal_atomic_load(&device_node->ref_count) > 0)
		osal_cond_wait(&g_lpf_device_ref_cond, &g_lpf_core_lock);
}

static void lpf_device_unregister_node(lpf_device_node_t *device_node)
{
	lpf_device_event_t event;

	if (!device_node)
		return;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node->removing = true;
	if (!list_empty(&device_node->node))
		list_del_init(&device_node->node);
	lpf_make_device_event(&device_node->device, LPF_DEVICE_EVENT_REMOVING,
			      OSAL_SUCCESS, &event);
	osal_mutex_unlock(&g_lpf_core_lock);

	lpf_emit_device_event(&event);

	osal_mutex_lock(&g_lpf_core_lock);
	lpf_device_wait_idle_locked(device_node);
	osal_mutex_unlock(&g_lpf_core_lock);

	if (device_node->device.driver && device_node->device.driver->remove)
		device_node->device.driver->remove(&device_node->device);

	lpf_emit_device_event_for_device(&device_node->device,
					 LPF_DEVICE_EVENT_REMOVED,
					 OSAL_SUCCESS);
	osal_free(device_node);
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
	info->error_count = device->error_count;
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

	if (osal_mutex_init(&g_lpf_event_lock, NULL) != OSAL_SUCCESS) {
		osal_mutex_destroy(&g_lpf_core_lock);
		return OSAL_ERR_GENERIC;
	}

	if (osal_cond_init(&g_lpf_device_ref_cond, NULL) != OSAL_SUCCESS) {
		osal_mutex_destroy(&g_lpf_event_lock);
		osal_mutex_destroy(&g_lpf_core_lock);
		return OSAL_ERR_GENERIC;
	}

	ret = lpf_soc_adapter_init();
	if (ret != OSAL_SUCCESS) {
		osal_cond_destroy(&g_lpf_device_ref_cond);
		osal_mutex_destroy(&g_lpf_event_lock);
		osal_mutex_destroy(&g_lpf_core_lock);
		return ret;
	}

	INIT_LIST_HEAD(&g_lpf_drivers);
	INIT_LIST_HEAD(&g_lpf_devices);
	INIT_LIST_HEAD(&g_lpf_event_subscribers);
	g_lpf_core_ready = true;

	ret = lpf_ctl_chrdev_register();
	if (ret != OSAL_SUCCESS) {
		g_lpf_core_ready = false;
		lpf_soc_adapter_deinit();
		osal_cond_destroy(&g_lpf_device_ref_cond);
		osal_mutex_destroy(&g_lpf_event_lock);
		osal_mutex_destroy(&g_lpf_core_lock);
		return ret;
	}

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
		device_node->removing = true;
		osal_mutex_unlock(&g_lpf_core_lock);

		lpf_device_unregister_node(device_node);
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
				device_node->removing = true;
				found = true;
				break;
			}
		}
		osal_mutex_unlock(&g_lpf_core_lock);

		if (!found)
			break;

		lpf_device_unregister_node(device_node);
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

	lpf_ctl_chrdev_unregister();
	lpf_driver_unregister_all();
	lpf_device_event_unsubscribe_all();
	lpf_soc_adapter_deinit();
	osal_cond_destroy(&g_lpf_device_ref_cond);
	osal_mutex_destroy(&g_lpf_event_lock);
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
	osal_atomic_init(&device_node->ref_count, 0);
	INIT_LIST_HEAD(&device_node->node);
	lpf_make_device_name(&device_node->device, driver);

	lpf_emit_device_event_for_device(&device_node->device,
					 LPF_DEVICE_EVENT_REGISTERED,
					 OSAL_SUCCESS);

	ret = driver->probe(&device_node->device);
	if (ret != OSAL_SUCCESS) {
		device_node->device.state = LPF_DEVICE_STATE_ERROR;
		device_node->device.last_error = ret;
		device_node->device.error_count++;
		lpf_emit_device_event_for_device(&device_node->device,
						 LPF_DEVICE_EVENT_ERROR, ret);
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

	lpf_emit_device_event_for_device(&device_node->device,
					 LPF_DEVICE_EVENT_BOUND,
					 OSAL_SUCCESS);

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

lpf_device_handle_t *lpf_device_get(lpf_device_type_t type, uint32_t index)
{
	lpf_device_node_t *device_node;
	lpf_device_handle_t *handle = NULL;

	if (type == LPF_DEVICE_TYPE_INVALID || !g_lpf_core_ready)
		return NULL;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_find_device_node_locked(type, index);
	handle = lpf_device_handle_create_locked(device_node);
	osal_mutex_unlock(&g_lpf_core_lock);

	return handle;
}
EXPORT_SYMBOL_GPL(lpf_device_get);

lpf_device_handle_t *lpf_device_get_by_name(const char *name)
{
	lpf_device_node_t *device_node;
	lpf_device_handle_t *handle = NULL;

	if (!name || !g_lpf_core_ready)
		return NULL;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_find_device_node_by_name_locked(name);
	handle = lpf_device_handle_create_locked(device_node);
	osal_mutex_unlock(&g_lpf_core_lock);

	return handle;
}
EXPORT_SYMBOL_GPL(lpf_device_get_by_name);

lpf_device_handle_t *
lpf_device_get_by_capability(lpf_capability_t required, uint32_t match_index)
{
	lpf_device_node_t *device_node;
	lpf_device_handle_t *handle = NULL;
	uint32_t matched_count = 0;

	if (required == LPF_DEVICE_CAP_NONE || !g_lpf_core_ready)
		return NULL;

	osal_mutex_lock(&g_lpf_core_lock);
	list_for_each_entry(device_node, &g_lpf_devices, node) {
		if ((device_node->device.capabilities & required) != required)
			continue;

		if (matched_count++ != match_index)
			continue;

		handle = lpf_device_handle_create_locked(device_node);
		break;
	}
	osal_mutex_unlock(&g_lpf_core_lock);

	return handle;
}
EXPORT_SYMBOL_GPL(lpf_device_get_by_capability);

const lpf_device_t *lpf_device_from_handle(
	const lpf_device_handle_t *handle)
{
	lpf_device_node_t *device_node;

	if (!handle)
		return NULL;

	device_node = handle->node;
	return device_node ? &device_node->device : NULL;
}
EXPORT_SYMBOL_GPL(lpf_device_from_handle);

int32_t lpf_device_handle_get_info(const lpf_device_handle_t *handle,
				   lpf_device_info_t *info)
{
	lpf_device_node_t *device_node;

	if (!handle || !info)
		return OSAL_ERR_INVALID_PARAM;
	if (!g_lpf_core_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_device_node_from_handle_locked(handle);
	if (!device_node) {
		osal_mutex_unlock(&g_lpf_core_lock);
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	lpf_copy_device_info(&device_node->device, info);
	osal_mutex_unlock(&g_lpf_core_lock);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_device_handle_get_info);

void lpf_device_put(lpf_device_handle_t *handle)
{
	lpf_device_node_t *device_node;
	uint32_t ref_count;

	if (!handle)
		return;

	if (g_lpf_core_ready) {
		osal_mutex_lock(&g_lpf_core_lock);
		device_node = lpf_device_node_from_handle_locked(handle);
		if (device_node &&
		    osal_atomic_load(&device_node->ref_count) > 0) {
			ref_count = osal_atomic_dec(&device_node->ref_count);
			if (ref_count == 0)
				osal_cond_broadcast(&g_lpf_device_ref_cond);
		}
		osal_mutex_unlock(&g_lpf_core_lock);
	}

	osal_free(handle);
}
EXPORT_SYMBOL_GPL(lpf_device_put);

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

int32_t lpf_device_set_state(lpf_device_type_t type, uint32_t index,
			     lpf_device_state_t state, int32_t status)
{
	lpf_device_node_t *device_node;
	lpf_device_event_t event;
	lpf_device_event_t error_event;
	bool emit_error = false;

	if (type == LPF_DEVICE_TYPE_INVALID)
		return OSAL_ERR_INVALID_PARAM;

	if (!g_lpf_core_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_find_device_node_locked(type, index);
	if (!device_node) {
		osal_mutex_unlock(&g_lpf_core_lock);
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	if (status != OSAL_SUCCESS) {
		state = LPF_DEVICE_STATE_ERROR;
		device_node->device.last_error = status;
		device_node->device.error_count++;
		emit_error = true;
	}
	device_node->device.state = state;
	lpf_make_device_event(&device_node->device,
			      LPF_DEVICE_EVENT_STATE_CHANGED, status, &event);
	if (emit_error)
		lpf_make_device_event(&device_node->device,
				      LPF_DEVICE_EVENT_ERROR, status,
				      &error_event);
	osal_mutex_unlock(&g_lpf_core_lock);

	lpf_emit_device_event(&event);
	if (emit_error)
		lpf_emit_device_event(&error_event);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_device_set_state);

void lpf_device_record_error(lpf_device_type_t type, uint32_t index,
			     int32_t error)
{
	lpf_device_node_t *device_node;
	lpf_device_event_t state_event;
	lpf_device_event_t error_event;
	bool emit_state_event = false;
	bool emit_error_event = false;

	if (error == OSAL_SUCCESS || type == LPF_DEVICE_TYPE_INVALID)
		return;
	if (!g_lpf_core_ready)
		return;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_find_device_node_locked(type, index);
	if (device_node) {
		device_node->device.last_error = error;
		device_node->device.error_count++;
		if (device_node->device.state != LPF_DEVICE_STATE_ERROR) {
			device_node->device.state = LPF_DEVICE_STATE_ERROR;
			lpf_make_device_event(&device_node->device,
					      LPF_DEVICE_EVENT_STATE_CHANGED,
					      error, &state_event);
			emit_state_event = true;
		}
		lpf_make_device_event(&device_node->device,
				      LPF_DEVICE_EVENT_ERROR, error,
				      &error_event);
		emit_error_event = true;
	}
	osal_mutex_unlock(&g_lpf_core_lock);

	if (emit_state_event)
		lpf_emit_device_event(&state_event);
	if (emit_error_event)
		lpf_emit_device_event(&error_event);
}
EXPORT_SYMBOL_GPL(lpf_device_record_error);

int32_t lpf_device_record_recovery(lpf_device_type_t type, uint32_t index)
{
	lpf_device_node_t *device_node;
	lpf_device_event_t event;
	bool emit_event = false;

	if (type == LPF_DEVICE_TYPE_INVALID)
		return OSAL_ERR_INVALID_PARAM;
	if (!g_lpf_core_ready)
		return OSAL_ERR_INVALID_STATE;

	osal_mutex_lock(&g_lpf_core_lock);
	device_node = lpf_find_device_node_locked(type, index);
	if (!device_node) {
		osal_mutex_unlock(&g_lpf_core_lock);
		return OSAL_ERR_NAME_NOT_FOUND;
	}

	if (device_node->device.state == LPF_DEVICE_STATE_ERROR) {
		device_node->device.state = LPF_DEVICE_STATE_BOUND;
		lpf_make_device_event(&device_node->device,
				      LPF_DEVICE_EVENT_STATE_CHANGED,
				      OSAL_SUCCESS, &event);
		emit_event = true;
	}
	osal_mutex_unlock(&g_lpf_core_lock);

	if (emit_event)
		lpf_emit_device_event(&event);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_device_record_recovery);

int32_t lpf_device_event_subscribe(lpf_device_event_callback_t callback,
				   void *user_data)
{
	lpf_device_event_subscriber_t *subscriber;
	int32_t ret;

	if (!callback)
		return OSAL_ERR_INVALID_PARAM;

	ret = lpf_core_init();
	if (ret != OSAL_SUCCESS)
		return ret;

	subscriber = osal_zalloc(sizeof(*subscriber));
	if (!subscriber)
		return OSAL_ERR_NO_MEMORY;

	INIT_LIST_HEAD(&subscriber->node);
	subscriber->notifier.notifier_call = lpf_device_event_notifier_call;
	subscriber->callback = callback;
	subscriber->user_data = user_data;

	osal_mutex_lock(&g_lpf_event_lock);
	if (lpf_find_event_subscriber_locked(callback, user_data)) {
		osal_mutex_unlock(&g_lpf_event_lock);
		osal_free(subscriber);
		return OSAL_ERR_ALREADY_EXISTS;
	}
	ret = blocking_notifier_chain_register(&g_lpf_device_notifier,
					       &subscriber->notifier);
	if (ret) {
		osal_mutex_unlock(&g_lpf_event_lock);
		osal_free(subscriber);
		return ret < 0 ? -ret : OSAL_ERR_GENERIC;
	}
	list_add_tail(&subscriber->node, &g_lpf_event_subscribers);
	osal_mutex_unlock(&g_lpf_event_lock);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_device_event_subscribe);

void lpf_device_event_unsubscribe(lpf_device_event_callback_t callback,
				  void *user_data)
{
	lpf_device_event_subscriber_t *subscriber;

	if (!callback || !g_lpf_core_ready)
		return;

	osal_mutex_lock(&g_lpf_event_lock);
	subscriber = lpf_find_event_subscriber_locked(callback, user_data);
	if (!subscriber) {
		osal_mutex_unlock(&g_lpf_event_lock);
		return;
	}
	list_del_init(&subscriber->node);
	osal_mutex_unlock(&g_lpf_event_lock);

	blocking_notifier_chain_unregister(&g_lpf_device_notifier,
					   &subscriber->notifier);
	osal_free(subscriber);
}
EXPORT_SYMBOL_GPL(lpf_device_event_unsubscribe);

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
