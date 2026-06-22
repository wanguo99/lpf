// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "../bus/pdm_bus_controller.h"
#include "pdm_ctl.h"
#include "../mock/pdm_mock_devices.h"
#include "pdm/core/pdm_client.h"
#include "pdm/core/pdm_backend.h"
#include "pdm/core/pdm_bus.h"
#include "pdm/pdm_errno.h"

/**
 * @brief PDM Core 模块初始化
 */
static int __init pdm_core_module_init(void)
{
	int ret;

	LOG_INFO("PDM_CORE", "Initializing PDM Core with Linux bus_type");

	ret = pdm_bus_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to initialize PDM bus: %d", ret);
		return ret;
	}

	ret = pdm_ctl_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to initialize PDM control node: %d", ret);
		goto err_bus;
	}

	ret = pdm_client_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to initialize PDM client nodes: %d", ret);
		goto err_ctl;
	}

	ret = pdm_driver_entries_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to register PDM drivers: %d", ret);
		goto err_client;
	}

	ret = pdm_backend_entries_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to initialize PDM backends: %d", ret);
		goto err_drivers;
	}

	ret = pdm_mock_devices_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to register mock PDM devices: %d", ret);
		goto err_backends;
	}

	ret = pdm_bus_controller_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to register bus controller: %d", ret);
		goto err_mock_devices;
	}

	LOG_INFO("PDM_CORE", "PDM Core initialized successfully");
	return 0;

err_mock_devices:
	pdm_mock_devices_exit();
err_backends:
	pdm_backend_entries_exit();
err_drivers:
	pdm_driver_entries_exit();
err_client:
	pdm_client_exit();
err_ctl:
	pdm_ctl_exit();
err_bus:
	pdm_bus_exit();
	return ret;
}

/**
 * @brief PDM Core 模块退出
 */
static void __exit pdm_core_module_exit(void)
{
	pdm_bus_controller_exit();
	pdm_mock_devices_exit();
	pdm_backend_entries_exit();
	pdm_driver_entries_exit();
	pdm_client_exit();
	pdm_ctl_exit();
	pdm_bus_exit();

	LOG_INFO("PDM_CORE", "PDM Core exited");
}

module_init(pdm_core_module_init);
module_exit(pdm_core_module_exit);

MODULE_AUTHOR("PDM");
MODULE_DESCRIPTION("PDM core device model with Linux bus_type");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal can can_raw");
