// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/lpf_core.h"
#include "lpf/lpf_errno.h"
#include "lpf/lpf_peripheral.h"
#include "pdm/pdm.h"
#include "pdm_ctl.h"
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

	ret = lpf_core_init();
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	ret = lpf_peripheral_services_init();
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	ret = lpf_peripheral_probe_devices();
	if (ret != OSAL_SUCCESS) {
		lpf_peripheral_services_exit();
		return lpf_status_to_errno(ret);
	}

	ret = pdm_ctl_chrdev_register();
	if (ret) {
		lpf_peripheral_services_exit();
		return ret;
	}

	LOG_INFO("PDM", "loaded");
	return 0;
}

static void __exit pdm_exit(void)
{
	pdm_ctl_chrdev_unregister();
	lpf_peripheral_services_exit();
	LOG_INFO("PDM", "unloaded");
}

module_init(pdm_init);
module_exit(pdm_exit);

MODULE_AUTHOR("LPF");
MODULE_DESCRIPTION("LPF PDM kernel module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal lpf_core pconfig hal can can_raw");
