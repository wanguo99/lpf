// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/lpf_errno.h"
#include "lpf/lpf_peripheral.h"

static int __init pdm_init(void)
{
	int ret;

	lpf_peripheral_runtime_print_version();

	ret = lpf_peripheral_runtime_init();
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	LOG_INFO("PDM", "loaded");
	return 0;
}

static void __exit pdm_exit(void)
{
	lpf_peripheral_runtime_exit();
	LOG_INFO("PDM", "unloaded");
}

module_init(pdm_init);
module_exit(pdm_exit);

MODULE_AUTHOR("LPF");
MODULE_DESCRIPTION("LPF peripheral integration module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal lpf_core pconfig hal can can_raw");
