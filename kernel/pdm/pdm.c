// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "core/bus/pdm_of_bus.h"
#include "drivers/mock/pdm_mock_devices.h"
#include "pdm/core/cdev/pdm_cdev.h"
#include "pdm/core/registry/pdm_backend.h"
#include "pdm/core/bus/pdm_bus.h"
#include "pdm/core/bus/pdm_device.h"
#include "pdm/pdm_errno.h"
#include "generated/gen_version.h"

static void pdm_print_version(void)
{
	LOG_INFO("================ PDM version info ================");
	LOG_INFO("pdm_version=%s", PDM_VERSION);
	LOG_INFO("git=%s", PDM_GIT_COMMIT);
	LOG_INFO("build_time=%s", PDM_COMPILE_TIME);
	LOG_INFO("build_by=%s@%s", PDM_COMPILE_BY, PDM_COMPILE_HOST);
	LOG_INFO("compiler=%s", PDM_COMPILER);
	LOG_INFO("arch=%s", PDM_BUILD_ARCH);
	LOG_INFO("kernel=%s", PDM_BUILD_KERNEL);
	LOG_INFO("==================================================");
}

/**
 * @brief PDM module 初始化
 */
static int __init pdm_module_init(void)
{
	int ret;

	pdm_print_version();

	LOG_INFO("Initializing PDM module with Linux bus_type");

	ret = pdm_bus_init();
	if (ret) {
		LOG_ERROR("Failed to initialize PDM bus: %d", ret);
		return ret;
	}

	ret = pdm_cdev_init();
	if (ret) {
		LOG_ERROR("Failed to initialize PDM character devices: %d", ret);
		goto err_bus;
	}

	ret = pdm_driver_entries_init();
	if (ret) {
		LOG_ERROR("Failed to register PDM drivers: %d", ret);
		goto err_cdev;
	}

	ret = pdm_backend_entries_init();
	if (ret) {
		LOG_ERROR("Failed to initialize PDM backends: %d", ret);
		goto err_drivers;
	}

	ret = pdm_mock_devices_init();
	if (ret) {
		LOG_ERROR("Failed to register mock PDM devices: %d", ret);
		goto err_backends;
	}

	ret = pdm_of_bus_init();
	if (ret) {
		LOG_ERROR("Failed to register OF bus enumerator: %d", ret);
		goto err_mock_devices;
	}

	LOG_INFO("PDM module initialized successfully");
	return 0;

err_mock_devices:
	pdm_mock_devices_exit();
err_backends:
	pdm_backend_entries_exit();
err_drivers:
	pdm_driver_entries_exit();
err_cdev:
	pdm_cdev_exit();
err_bus:
	pdm_bus_exit();
	return ret;
}

/**
 * @brief PDM module 退出
 */
static void __exit pdm_module_exit(void)
{
	pdm_of_bus_exit();
	pdm_mock_devices_exit();
	pdm_backend_entries_exit();
	pdm_driver_entries_exit();
	pdm_cdev_exit();
	pdm_bus_exit();
	pdm_device_ids_destroy();

	LOG_INFO("PDM module exited");
}

module_init(pdm_module_init);
module_exit(pdm_module_exit);

MODULE_AUTHOR("PDM");
MODULE_DESCRIPTION("PDM core device model with Linux bus_type");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal can can_raw");
