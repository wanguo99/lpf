// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "testing/pdm_mock_devices.h"
#include "pdm/bus/pdm_cdev.h"
#include "pdm/registry/pdm_backend.h"
#include "pdm/bus/pdm_bus.h"
#include "pdm/bus/pdm_device.h"
#include "generated/gen_version.h"
#include "pdm/log/pdm_log.h"

static void pdm_print_version(void)
{
	LOG_INFO("================ PAF/PDM version info ================");
	LOG_INFO("paf_version=%s", PAF_VERSION);
	LOG_INFO("git=%s", PAF_GIT_COMMIT);
	LOG_INFO("build_time=%s", PAF_COMPILE_TIME);
	LOG_INFO("build_by=%s@%s", PAF_COMPILE_BY, PAF_COMPILE_HOST);
	LOG_INFO("compiler=%s", PAF_COMPILER);
	LOG_INFO("arch=%s", PAF_BUILD_ARCH);
	LOG_INFO("kernel=%s", PAF_BUILD_KERNEL);
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

	ret = pdm_device_ids_init();
	if (ret) {
		LOG_ERROR("Failed to initialize PDM ID management: %d", ret);
		return ret;
	}

	ret = pdm_bus_init();
	if (ret) {
		LOG_ERROR("Failed to initialize PDM bus: %d", ret);
		goto err_id;
	}

	ret = pdm_cdev_init();
	if (ret) {
		LOG_ERROR("Failed to initialize PDM character devices: %d", ret);
		goto err_bus;
	}

	ret = pdm_backend_entries_init();
	if (ret) {
		LOG_ERROR("Failed to initialize PDM backends: %d", ret);
		goto err_cdev;
	}

	ret = pdm_driver_entries_init();
	if (ret) {
		LOG_ERROR("Failed to register PDM drivers: %d", ret);
		goto err_backends;
	}

	ret = pdm_mock_devices_init();
	if (ret) {
		LOG_ERROR("Failed to register mock PDM devices: %d", ret);
		goto err_drivers;
	}

	LOG_INFO("PDM module initialized successfully");
	return 0;

err_drivers:
	pdm_driver_entries_exit();
err_backends:
	pdm_backend_entries_exit();
err_cdev:
	pdm_cdev_exit();
err_bus:
	pdm_bus_exit();
err_id:
	pdm_device_ids_destroy();
	return ret;
}

/**
 * @brief PDM module 退出
 */
static void __exit pdm_module_exit(void)
{
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
#ifdef CONFIG_PDM_MCU_CAN
MODULE_SOFTDEP("pre: can can_raw");
#endif
