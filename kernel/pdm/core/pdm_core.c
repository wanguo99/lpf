// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "pdm/core/pdm_bus.h"
#include "pdm/pdm_errno.h"

/* Forward declarations for bus controller */
extern int __init pdm_bus_controller_init(void);
extern void __exit pdm_bus_controller_exit(void);

/**
 * @brief PDM Core 模块初始化
 */
static int __init pdm_core_module_init(void)
{
	int ret;

	LOG_INFO("PDM_CORE", "Initializing PDM Core with Linux bus_type");

	/* 初始化 Linux bus_type */
	ret = pdm_bus_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to initialize PDM bus: %d", ret);
		return ret;
	}

	/* 注册 PDM bus controller platform driver */
	ret = pdm_bus_controller_init();
	if (ret) {
		LOG_ERROR("PDM_CORE", "Failed to register bus controller: %d", ret);
		pdm_bus_exit();
		return ret;
	}

	LOG_INFO("PDM_CORE", "PDM Core initialized successfully");
	return 0;
}

/**
 * @brief PDM Core 模块退出
 */
static void __exit pdm_core_module_exit(void)
{
	pdm_bus_controller_exit();
	pdm_bus_exit();

	LOG_INFO("PDM_CORE", "PDM Core exited");
}

module_init(pdm_core_module_init);
module_exit(pdm_core_module_exit);

MODULE_AUTHOR("PDM");
MODULE_DESCRIPTION("PDM core device model with Linux bus_type");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal can can_raw");
