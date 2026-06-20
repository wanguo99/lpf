// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/lpf_errno.h"
#include "lpf/lpf_peripheral.h"
#include "pdm/pdm.h"
#include "generated/gen_version.h"

void pdm_print_version(void)
{
	osal_log(OS_LOG_LEVEL_INFO, "PDM",
		 "module_version=%u.%u.%u lpf_version=%s git=%s build_time=%s build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 PDM_VERSION_MAJOR, PDM_VERSION_MINOR, PDM_VERSION_PATCH,
		 LPF_VERSION, LPF_GIT_COMMIT,
		 LPF_COMPILE_TIME, LPF_COMPILE_BY,
		 LPF_COMPILE_HOST, LPF_COMPILER,
		 LPF_BUILD_ARCH, LPF_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(pdm_print_version);

static int __init pdm_init(void)
{
	int ret;

	pdm_print_version();

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
MODULE_DESCRIPTION("LPF PDM kernel module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal lpf_core pconfig hal can can_raw");
