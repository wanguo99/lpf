// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "hal/hal.h"

static int __init hal_init(void)
{
	pr_info("es-middleware-hal: loaded\n");
	return 0;
}

static void __exit hal_exit(void)
{
	pr_info("es-middleware-hal: unloaded\n");
}

module_init(hal_init);
module_exit(hal_exit);

MODULE_AUTHOR("ES-Middleware");
MODULE_DESCRIPTION("ES-Middleware HAL kernel module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal can can_raw");
