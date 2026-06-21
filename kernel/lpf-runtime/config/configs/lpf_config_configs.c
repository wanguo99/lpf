// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf_config_static.h"

#ifdef CONFIG_LPF_CONFIG_KERNEL_X86_MODULES
extern const lpf_config_platform_config_t g_lpf_config_kernel_x86_modules_v1;
#endif

#ifdef CONFIG_LPF_CONFIG_KERNEL_X86_MOCK_MODULES
extern const lpf_config_platform_config_t
	g_lpf_config_kernel_x86_mock_modules_v1;
#endif

static const lpf_config_platform_config_t *const g_lpf_config_configs[] = {
#ifdef CONFIG_LPF_CONFIG_KERNEL_X86_MODULES
	&g_lpf_config_kernel_x86_modules_v1,
#endif
#ifdef CONFIG_LPF_CONFIG_KERNEL_X86_MOCK_MODULES
	&g_lpf_config_kernel_x86_mock_modules_v1,
#endif
};

const lpf_config_static_table_t g_lpf_config_platform_table = {
	.configs = g_lpf_config_configs,
	.count = OSAL_ARRAY_SIZE(g_lpf_config_configs),
	.current_index = 0,
};
EXPORT_SYMBOL_GPL(g_lpf_config_platform_table);

static int __init lpf_configs_module_init(void)
{
	LOG_INFO("LPF_CONFIGS", "loaded %u static config(s)",
		 g_lpf_config_platform_table.count);
	return 0;
}

static void __exit lpf_configs_module_exit(void)
{
	LOG_INFO("LPF_CONFIGS", "unloaded");
}

module_init(lpf_configs_module_init);
module_exit(lpf_configs_module_exit);

MODULE_AUTHOR("LPF");
MODULE_DESCRIPTION("LPF selected static platform configuration");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal");
