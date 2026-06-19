// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal/osal.h"

static int __init osal_init(void)
{
	osal_print_module_version("OSAL");
	LOG_INFO("OSAL", "loaded");
	return 0;
}

static void __exit osal_exit(void)
{
	LOG_INFO("OSAL", "unloaded");
}

module_init(osal_init);
module_exit(osal_exit);

MODULE_AUTHOR("ES-Middleware");
MODULE_DESCRIPTION("ES-Middleware OSAL kernel module");
MODULE_LICENSE("GPL");
