// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/config/lpf_config.h"
#include "lpf/config/lpf_config_static.h"

extern const lpf_config_platform_config_t *const lpf_config_static_start;
extern const lpf_config_platform_config_t *const lpf_config_static_end;

lpf_config_static_table_t g_lpf_config_platform_table = {
	.configs = &lpf_config_static_start + 1,
	.current_index = 0,
};
EXPORT_SYMBOL_GPL(g_lpf_config_platform_table);

static uint32_t lpf_configs_count_static_configs(void)
{
	return (uint32_t)(&lpf_config_static_end -
			  &lpf_config_static_start - 1);
}

static int __init lpf_configs_module_init(void)
{
	g_lpf_config_platform_table.count = lpf_configs_count_static_configs();
	lpf_config_print_version();
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
MODULE_DESCRIPTION("LPF configuration engine and built-in data providers");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal");
