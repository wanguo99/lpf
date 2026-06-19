// SPDX-License-Identifier: GPL-2.0

#include "osal/osal.h"
#include "generated/gen_version.h"

#include <linux/module.h>

void osal_print_module_version(const char *module_name)
{
	const char *name = module_name ? module_name : "UNKNOWN";

	osal_log(OS_LOG_LEVEL_INFO, name,
		 "version=%s git=%s build_time=%s build_by=%s@%s compiler=%s arch=%s kernel=%s",
		 ES_MIDDLEWARE_VERSION, ES_MIDDLEWARE_GIT_COMMIT,
		 ES_MIDDLEWARE_COMPILE_TIME, ES_MIDDLEWARE_COMPILE_BY,
		 ES_MIDDLEWARE_COMPILE_HOST, ES_MIDDLEWARE_COMPILER,
		 ES_MIDDLEWARE_BUILD_ARCH, ES_MIDDLEWARE_BUILD_KERNEL);
}
EXPORT_SYMBOL_GPL(osal_print_module_version);
