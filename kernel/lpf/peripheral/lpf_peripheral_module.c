// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/lpf_errno.h"
#include "lpf/lpf_peripheral.h"

static int __init lpf_peripheral_module_init(void)
{
	int ret;

	lpf_peripheral_runtime_print_version();

	ret = lpf_peripheral_runtime_init();
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	LOG_INFO("LPF-PERIPHERAL", "loaded");
	return 0;
}

static void __exit lpf_peripheral_module_exit(void)
{
	lpf_peripheral_runtime_exit();
	LOG_INFO("LPF-PERIPHERAL", "unloaded");
}

module_init(lpf_peripheral_module_init);
module_exit(lpf_peripheral_module_exit);

MODULE_AUTHOR("LPF");
MODULE_DESCRIPTION("LPF peripheral integration module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal lpf_core pconfig hal can can_raw");
