// SPDX-License-Identifier: GPL-2.0

#include "pdm/runtime/pdm_runtime.h"

#include "pdm/core/pdm_core.h"
#include "pdm/hw/pdm_hw.h"
/* #include "pdm/config/pdm_config.h" - 已删除：使用 Device Tree 替代 */
#include "pdm_runtime_internal.h"
#include "generated/gen_version.h"

static bool g_lpf_runtime_ready;
static bool g_lpf_runtime_entries_ready;
static bool g_lpf_runtime_devices_ready;

void pdm_runtime_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "========================================");
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "module_version : %u.%u.%u",
		 PDM_RUNTIME_VERSION_MAJOR,
		 PDM_RUNTIME_VERSION_MINOR,
		 PDM_RUNTIME_VERSION_PATCH);
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "pdm_version    : %s", PDM_VERSION);
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "git_commit     : %s", PDM_GIT_COMMIT);
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "build_time     : %s", PDM_COMPILE_TIME);
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "build_by       : %s@%s",
		 PDM_COMPILE_BY, PDM_COMPILE_HOST);
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "compiler       : %s", PDM_COMPILER);
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "arch           : %s", PDM_BUILD_ARCH);
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "kernel         : %s", PDM_BUILD_KERNEL);
	osal_log(OS_LOG_LEVEL_INFO, "PDM-RUNTIME",
		 "========================================");
}

static const pdm_runtime_entry_t *pdm_runtime_entry_first(void)
{
	return &pdm_runtime_entry_start + 1;
}

static const pdm_runtime_entry_t *pdm_runtime_entry_last(void)
{
	return &pdm_runtime_entry_end;
}

static void pdm_runtime_entries_exit_range(
	pdm_runtime_entry_class_t class, const pdm_runtime_entry_t *end)
{
	const pdm_runtime_entry_t *entry;

	entry = end;
	while (entry > pdm_runtime_entry_first()) {
		entry--;
		if (entry->class != class)
			continue;
		if (entry->exit)
			entry->exit();
	}
}

static void
pdm_runtime_entries_exit_initialized(pdm_runtime_entry_class_t last_class,
				     const pdm_runtime_entry_t *end)
{
	int class;

	class = (int)last_class;
	for (; class >= 0; class--)
		pdm_runtime_entries_exit_range(
			(pdm_runtime_entry_class_t)class,
			class == (int)last_class ? end :
				pdm_runtime_entry_last());
}

static int32_t
pdm_runtime_entries_init_class(pdm_runtime_entry_class_t class)
{
	const pdm_runtime_entry_t *entry;
	int32_t ret;

	for (entry = pdm_runtime_entry_first();
	     entry < pdm_runtime_entry_last(); entry++) {
		if (entry->class != class)
			continue;
		if (!entry->init)
			continue;

		ret = entry->init();
		if (ret != OSAL_SUCCESS) {
			LOG_ERROR("PDM-RUNTIME",
				  "entry %s class=%u init failed: %d",
				  entry->name ? entry->name : "unknown",
				  entry->class, ret);
			pdm_runtime_entries_exit_initialized(class, entry);
			return ret;
		}

		LOG_INFO("PDM-RUNTIME", "entry %s class=%u initialized",
			 entry->name ? entry->name : "unknown",
			 entry->class);
	}

	return OSAL_SUCCESS;
}

static int32_t pdm_runtime_entries_init(void)
{
	pdm_runtime_entry_class_t class;
	int32_t ret;

	if (g_lpf_runtime_entries_ready)
		return OSAL_SUCCESS;

	for (class = PDM_RUNTIME_ENTRY_CLASS_CORE;
	     class < PDM_RUNTIME_ENTRY_CLASS_COUNT; class++) {
		ret = pdm_runtime_entries_init_class(class);
		if (ret != OSAL_SUCCESS)
			return ret;
	}

	g_lpf_runtime_entries_ready = true;
	return OSAL_SUCCESS;
}

static void pdm_runtime_entries_exit(void)
{
	if (!g_lpf_runtime_entries_ready)
		return;

	pdm_runtime_config_detach();
	pdm_runtime_entries_exit_initialized(
		(pdm_runtime_entry_class_t)(PDM_RUNTIME_ENTRY_CLASS_COUNT - 1),
		pdm_runtime_entry_last());
	g_lpf_runtime_entries_ready = false;
}

int32_t pdm_runtime_config_refresh(void)
{
	if (!g_lpf_runtime_ready && !g_lpf_runtime_entries_ready)
		return OSAL_ERR_INVALID_STATE;

	if (g_lpf_runtime_devices_ready)
		return OSAL_SUCCESS;

	/*
	 * 注意：pdm_runtime_probe_devices() 已禁用
	 * 原因：依赖已删除的 pdm_configs 模块
	 *
	 * 在新总线架构中：
	 * - 设备由 pdm_bus_controller 从 Device Tree 创建
	 * - 驱动通过 pdm_driver_register() 注册到 pdm_bus
	 * - 内核自动进行匹配和 probe
	 *
	 * 此函数现在是空操作，保留以维持 API 兼容性
	 */

	g_lpf_runtime_devices_ready = true;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(pdm_runtime_config_refresh);

void pdm_runtime_config_detach(void)
{
	if (!g_lpf_runtime_devices_ready)
		return;

	/* 注意: 在新总线架构中，设备由 Device Tree 创建和管理 */
	/* pdm_device_unregister_all(); - 旧伪总线 API 已删除 */
	/* pdm_config_unload(); - pdm_configs 模块已删除 */
	g_lpf_runtime_devices_ready = false;
}
EXPORT_SYMBOL_GPL(pdm_runtime_config_detach);

int32_t pdm_runtime_init(void)
{
	int32_t ret;

	if (g_lpf_runtime_ready)
		return OSAL_SUCCESS;

	ret = pdm_hw_runtime_init();
	if (ret != OSAL_SUCCESS)
		return ret;

	ret = pdm_runtime_entries_init();
	if (ret != OSAL_SUCCESS) {
		pdm_hw_runtime_exit();
		return ret;
	}

	ret = pdm_runtime_config_refresh();
	if (ret != OSAL_SUCCESS) {
		pdm_runtime_entries_exit();
		pdm_hw_runtime_exit();
		return ret;
	}

	g_lpf_runtime_ready = true;
	return OSAL_SUCCESS;
}

void pdm_runtime_exit(void)
{
	if (!g_lpf_runtime_ready)
		return;

	pdm_runtime_entries_exit();
	pdm_hw_runtime_exit();
	g_lpf_runtime_ready = false;
}
