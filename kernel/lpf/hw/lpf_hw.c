// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/lpf_hw.h"
#include "lpf_hw_internal.h"
#include "generated/gen_version.h"

static bool g_lpf_hw_builtin_drivers_ready;

static const lpf_hw_builtin_driver_t *lpf_hw_builtin_driver_first(void)
{
	return &lpf_hw_builtin_driver_start + 1;
}

static const lpf_hw_builtin_driver_t *lpf_hw_builtin_driver_last(void)
{
	return &lpf_hw_builtin_driver_end;
}

static void
lpf_hw_builtin_drivers_exit_range(const lpf_hw_builtin_driver_t *end)
{
	const lpf_hw_builtin_driver_t *first = lpf_hw_builtin_driver_first();
	const lpf_hw_builtin_driver_t *driver = end;

	while (driver > first) {
		driver--;
		if (driver->exit)
			driver->exit();
	}
}

static int lpf_hw_builtin_drivers_init(void)
{
	const lpf_hw_builtin_driver_t *driver;
	int ret;

	for (driver = lpf_hw_builtin_driver_first();
	     driver < lpf_hw_builtin_driver_last(); driver++) {
		if (!driver->init)
			continue;

		ret = driver->init();
		if (ret) {
			LOG_ERROR("LPF_HW", "Builtin driver %s init failed: %d",
				  driver->name ? driver->name : "unknown", ret);
			lpf_hw_builtin_drivers_exit_range(driver);
			return ret;
		}

		LOG_INFO("LPF_HW", "Builtin driver %s initialized",
			 driver->name ? driver->name : "unknown");
	}

	g_lpf_hw_builtin_drivers_ready = true;
	return 0;
}

static void lpf_hw_builtin_drivers_exit(void)
{
	if (!g_lpf_hw_builtin_drivers_ready)
		return;

	lpf_hw_builtin_drivers_exit_range(lpf_hw_builtin_driver_last());
	g_lpf_hw_builtin_drivers_ready = false;
}

void lpf_hw_runtime_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "LPF_HW",
		 "module_version=%u.%u.%u lpf_version=%s git=%s build_time=%s "
		 "build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 LPF_HW_VERSION_MAJOR, LPF_HW_VERSION_MINOR, LPF_HW_VERSION_PATCH,
		 LPF_VERSION, LPF_GIT_COMMIT,
		 LPF_COMPILE_TIME, LPF_COMPILE_BY,
		 LPF_COMPILE_HOST, LPF_COMPILER,
		 LPF_BUILD_ARCH, LPF_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(lpf_hw_runtime_print_version);

int32_t lpf_hw_runtime_init(void)
{
	int ret;

	lpf_hw_runtime_print_version();

	ret = lpf_hw_builtin_drivers_init();
	if (ret)
		return ret;

	LOG_INFO("LPF_HW", "runtime initialized");
	return OSAL_SUCCESS;
}

void lpf_hw_runtime_exit(void)
{
	lpf_hw_builtin_drivers_exit();
	LOG_INFO("LPF_HW", "runtime exited");
}
