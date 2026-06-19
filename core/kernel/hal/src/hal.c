// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "hal/hal.h"
#include "hal_internal.h"

static int __init hal_init(void)
{
	osal_print_module_version("HAL");
#ifdef CONFIG_HAL_GPIO
	int ret;

	ret = hal_gpio_module_init();
	if (ret)
		return ret;
#endif

	LOG_INFO("HAL", "loaded");
	return 0;
}

static void __exit hal_exit(void)
{
#ifdef CONFIG_HAL_GPIO
	hal_gpio_module_deinit();
#endif
	LOG_INFO("HAL", "unloaded");
}

module_init(hal_init);
module_exit(hal_exit);

MODULE_AUTHOR("ES-Middleware");
MODULE_DESCRIPTION("ES-Middleware HAL kernel module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal can can_raw");
