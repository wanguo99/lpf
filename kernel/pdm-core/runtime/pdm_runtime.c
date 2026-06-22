// SPDX-License-Identifier: GPL-2.0

#include "lpf/runtime/lpf_runtime.h"

#include "lpf/core/lpf_core.h"
#include "lpf/hw/lpf_hw.h"
#include "lpf/config/lpf_config.h"
#include "lpf_runtime_internal.h"
#include "generated/gen_version.h"

static bool g_lpf_runtime_ready;
static bool g_lpf_runtime_entries_ready;
static bool g_lpf_runtime_devices_ready;

void lpf_runtime_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "========================================");
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "module_version : %u.%u.%u",
		 LPF_RUNTIME_VERSION_MAJOR,
		 LPF_RUNTIME_VERSION_MINOR,
		 LPF_RUNTIME_VERSION_PATCH);
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "lpf_version    : %s", LPF_VERSION);
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "git_commit     : %s", LPF_GIT_COMMIT);
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "build_time     : %s", LPF_COMPILE_TIME);
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "build_by       : %s@%s",
		 LPF_COMPILE_BY, LPF_COMPILE_HOST);
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "compiler       : %s", LPF_COMPILER);
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "arch           : %s", LPF_BUILD_ARCH);
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "kernel         : %s", LPF_BUILD_KERNEL);
	osal_log(OS_LOG_LEVEL_INFO, "LPF-RUNTIME",
		 "========================================");
}

static const lpf_runtime_entry_t *lpf_runtime_entry_first(void)
{
	return &lpf_runtime_entry_start + 1;
}

static const lpf_runtime_entry_t *lpf_runtime_entry_last(void)
{
	return &lpf_runtime_entry_end;
}

static void lpf_runtime_entries_exit_range(
	lpf_runtime_entry_class_t class, const lpf_runtime_entry_t *end)
{
	const lpf_runtime_entry_t *entry;

	entry = end;
	while (entry > lpf_runtime_entry_first()) {
		entry--;
		if (entry->class != class)
			continue;
		if (entry->exit)
			entry->exit();
	}
}

static void
lpf_runtime_entries_exit_initialized(lpf_runtime_entry_class_t last_class,
				     const lpf_runtime_entry_t *end)
{
	int class;

	class = (int)last_class;
	for (; class >= 0; class--)
		lpf_runtime_entries_exit_range(
			(lpf_runtime_entry_class_t)class,
			class == (int)last_class ? end :
				lpf_runtime_entry_last());
}

static int32_t
lpf_runtime_entries_init_class(lpf_runtime_entry_class_t class)
{
	const lpf_runtime_entry_t *entry;
	int32_t ret;

	for (entry = lpf_runtime_entry_first();
	     entry < lpf_runtime_entry_last(); entry++) {
		if (entry->class != class)
			continue;
		if (!entry->init)
			continue;

		ret = entry->init();
		if (ret != OSAL_SUCCESS) {
			LOG_ERROR("LPF-RUNTIME",
				  "entry %s class=%u init failed: %d",
				  entry->name ? entry->name : "unknown",
				  entry->class, ret);
			lpf_runtime_entries_exit_initialized(class, entry);
			return ret;
		}

		LOG_INFO("LPF-RUNTIME", "entry %s class=%u initialized",
			 entry->name ? entry->name : "unknown",
			 entry->class);
	}

	return OSAL_SUCCESS;
}

static int32_t lpf_runtime_entries_init(void)
{
	lpf_runtime_entry_class_t class;
	int32_t ret;

	if (g_lpf_runtime_entries_ready)
		return OSAL_SUCCESS;

	for (class = LPF_RUNTIME_ENTRY_CLASS_CORE;
	     class < LPF_RUNTIME_ENTRY_CLASS_COUNT; class++) {
		ret = lpf_runtime_entries_init_class(class);
		if (ret != OSAL_SUCCESS)
			return ret;
	}

	g_lpf_runtime_entries_ready = true;
	return OSAL_SUCCESS;
}

static void lpf_runtime_entries_exit(void)
{
	if (!g_lpf_runtime_entries_ready)
		return;

	lpf_runtime_config_detach();
	lpf_runtime_entries_exit_initialized(
		(lpf_runtime_entry_class_t)(LPF_RUNTIME_ENTRY_CLASS_COUNT - 1),
		lpf_runtime_entry_last());
	g_lpf_runtime_entries_ready = false;
}

int32_t lpf_runtime_config_refresh(void)
{
	int32_t ret;

	if (!g_lpf_runtime_ready && !g_lpf_runtime_entries_ready)
		return OSAL_ERR_INVALID_STATE;

	if (g_lpf_runtime_devices_ready)
		return OSAL_SUCCESS;

	ret = lpf_runtime_probe_devices();
	if (ret != OSAL_SUCCESS)
		return ret;

	g_lpf_runtime_devices_ready = true;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_runtime_config_refresh);

void lpf_runtime_config_detach(void)
{
	if (!g_lpf_runtime_devices_ready)
		return;

	lpf_device_unregister_all();
	lpf_config_unload();
	g_lpf_runtime_devices_ready = false;
}
EXPORT_SYMBOL_GPL(lpf_runtime_config_detach);

int32_t lpf_runtime_init(void)
{
	int32_t ret;

	if (g_lpf_runtime_ready)
		return OSAL_SUCCESS;

	ret = lpf_hw_runtime_init();
	if (ret != OSAL_SUCCESS)
		return ret;

	ret = lpf_runtime_entries_init();
	if (ret != OSAL_SUCCESS) {
		lpf_hw_runtime_exit();
		return ret;
	}

	ret = lpf_runtime_config_refresh();
	if (ret != OSAL_SUCCESS) {
		lpf_runtime_entries_exit();
		lpf_hw_runtime_exit();
		return ret;
	}

	g_lpf_runtime_ready = true;
	return OSAL_SUCCESS;
}

void lpf_runtime_exit(void)
{
	if (!g_lpf_runtime_ready)
		return;

	lpf_runtime_entries_exit();
	lpf_hw_runtime_exit();
	g_lpf_runtime_ready = false;
}
