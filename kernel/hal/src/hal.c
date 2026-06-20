// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "hal/hal.h"
#include "hal_internal.h"
#include "generated/gen_version.h"

static bool g_hal_builtin_drivers_ready;

static const hal_builtin_driver_t *hal_builtin_driver_first(void)
{
	return &hal_builtin_driver_start + 1;
}

static const hal_builtin_driver_t *hal_builtin_driver_last(void)
{
	return &hal_builtin_driver_end;
}

static void
hal_builtin_drivers_exit_range(const hal_builtin_driver_t *end)
{
	const hal_builtin_driver_t *first = hal_builtin_driver_first();
	const hal_builtin_driver_t *driver = end;

	while (driver > first) {
		driver--;
		if (driver->exit)
			driver->exit();
	}
}

static int hal_builtin_drivers_init(void)
{
	const hal_builtin_driver_t *driver;
	int ret;

	for (driver = hal_builtin_driver_first();
	     driver < hal_builtin_driver_last(); driver++) {
		if (!driver->init)
			continue;

		ret = driver->init();
		if (ret) {
			LOG_ERROR("HAL", "Builtin driver %s init failed: %d",
				  driver->name ? driver->name : "unknown", ret);
			hal_builtin_drivers_exit_range(driver);
			return ret;
		}

		LOG_INFO("HAL", "Builtin driver %s initialized",
			 driver->name ? driver->name : "unknown");
	}

	g_hal_builtin_drivers_ready = true;
	return 0;
}

static void hal_builtin_drivers_exit(void)
{
	if (!g_hal_builtin_drivers_ready)
		return;

	hal_builtin_drivers_exit_range(hal_builtin_driver_last());
	g_hal_builtin_drivers_ready = false;
}

void hal_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "HAL",
		 "module_version=%u.%u.%u lpf_version=%s git=%s build_time=%s build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 HAL_VERSION_MAJOR, HAL_VERSION_MINOR, HAL_VERSION_PATCH,
		 LPF_VERSION, LPF_GIT_COMMIT,
		 LPF_COMPILE_TIME, LPF_COMPILE_BY,
		 LPF_COMPILE_HOST, LPF_COMPILER,
		 LPF_BUILD_ARCH, LPF_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(hal_print_version);

int32_t hal_runtime_init(void)
{
	int ret;

	hal_print_version();

	ret = hal_builtin_drivers_init();
	if (ret)
		return ret;

	LOG_INFO("HAL", "runtime initialized");
	return OSAL_SUCCESS;
}

void hal_runtime_exit(void)
{
	hal_builtin_drivers_exit();
	LOG_INFO("HAL", "runtime exited");
}
