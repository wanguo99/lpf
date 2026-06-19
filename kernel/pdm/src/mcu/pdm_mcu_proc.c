// SPDX-License-Identifier: GPL-2.0

#include "pdm_mcu_internal.h"
#include "pdm_proc.h"

static pdm_proc_entry_t g_pdm_mcu_proc;

static const char *pdm_mcu_interface_name(pconfig_mcu_interface_t interface)
{
	switch (interface) {
	case PCONFIG_MCU_INTERFACE_CAN:
		return "can";
	case PCONFIG_MCU_INTERFACE_SERIAL:
		return "serial";
	default:
		return "unknown";
	}
}

static int pdm_mcu_proc_show(struct seq_file *seq, void *data)
{
	pdm_mcu_debug_info_t info;
	uint32_t i;
	int32_t ret;

	(void)data;

	seq_puts(seq, "PDM MCU\n");
	seq_printf(seq, "max_devices=%u\n", PDM_MCU_MAX_DEVICES);
	seq_puts(seq, "devices:\n");

	for (i = 0; i < PDM_MCU_MAX_DEVICES; i++) {
		ret = pdm_mcu_debug_get(i, &info);
		if (ret == OSAL_ERR_INVALID_STATE) {
			seq_puts(seq, "  registry=uninitialized\n");
			return 0;
		}
		if (ret != OSAL_SUCCESS)
			continue;

		if (!info.present) {
			seq_printf(seq, "  [%u] present=0\n", i);
			continue;
		}

		seq_printf(seq,
			   "  [%u] present=1 name=%s interface=%s timeout_ms=%u\n",
			   i, info.name,
			   pdm_mcu_interface_name(info.interface),
			   info.cmd_timeout_ms);
	}

	return 0;
}

int pdm_mcu_proc_register(void)
{
	return pdm_proc_register(&g_pdm_mcu_proc, "mcu",
				 pdm_mcu_proc_show, NULL);
}

void pdm_mcu_proc_unregister(void)
{
	pdm_proc_unregister(&g_pdm_mcu_proc);
}
