// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "pdm_mcu/pdm_mcu_internal.h"
#ifdef CONFIG_PDM_PROTOCOL
#include "pdm_protocol.h"
#endif

static int __init pdm_init(void)
{
	int ret;

#ifdef CONFIG_PDM_PROTOCOL
	ret = pdm_protocol_init();
	if (ret)
		return ret;
#endif

#ifdef CONFIG_PDM_MCU_SUPPORT
	ret = pdm_mcu_chrdev_register();
	if (ret) {
#ifdef CONFIG_PDM_PROTOCOL
		pdm_protocol_deinit();
#endif
		return ret;
	}
#else
	ret = 0;
#endif

	LOG_INFO("PDM", "loaded");
	return 0;
}

static void __exit pdm_exit(void)
{
#ifdef CONFIG_PDM_MCU_SUPPORT
	pdm_mcu_chrdev_unregister();
#endif
#ifdef CONFIG_PDM_PROTOCOL
	pdm_protocol_deinit();
#endif
	LOG_INFO("PDM", "unloaded");
}

module_init(pdm_init);
module_exit(pdm_exit);

MODULE_AUTHOR("ES-Middleware");
MODULE_DESCRIPTION("ES-Middleware PDM kernel module");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal hal can can_raw");
