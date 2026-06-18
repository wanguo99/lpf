// SPDX-License-Identifier: GPL-2.0

#include "osal/osal_kernel.h"

#include <linux/module.h>

const char *osal_kernel_version(void)
{
	return "kernel-osal-0.1";
}
EXPORT_SYMBOL_GPL(osal_kernel_version);

MODULE_AUTHOR("ES-Middleware");
MODULE_DESCRIPTION("ES-Middleware OSAL kernel module");
MODULE_LICENSE("GPL");
