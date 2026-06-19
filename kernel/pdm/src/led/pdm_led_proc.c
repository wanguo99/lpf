// SPDX-License-Identifier: GPL-2.0

#include "pdm_led_internal.h"
#include "pdm_proc.h"

static pdm_proc_entry_t g_pdm_led_proc;

static const char *pdm_led_control_name(pconfig_led_control_t control)
{
	switch (control) {
	case PCONFIG_LED_CONTROL_GPIO:
		return "gpio";
	case PCONFIG_LED_CONTROL_PWM:
		return "pwm";
	default:
		return "unknown";
	}
}

static int pdm_led_proc_show(struct seq_file *seq, void *data)
{
	pdm_led_debug_info_t info;
	uint32_t i;
	int32_t ret;

	(void)data;

	seq_puts(seq, "PDM LED\n");
	seq_printf(seq, "max_devices=%u\n", PDM_LED_MAX_DEVICES);
	seq_puts(seq, "devices:\n");

	for (i = 0; i < PDM_LED_MAX_DEVICES; i++) {
		ret = pdm_led_debug_get(i, &info);
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
			   "  [%u] present=1 name=%s control=%s enabled=%u brightness=%u max_brightness=%u\n",
			   i, info.name, pdm_led_control_name(info.control),
			   info.enabled ? 1U : 0U, info.brightness,
			   info.max_brightness);
	}

	return 0;
}

int pdm_led_proc_register(void)
{
	return pdm_proc_register(&g_pdm_led_proc, "led",
				 pdm_led_proc_show, NULL);
}

void pdm_led_proc_unregister(void)
{
	pdm_proc_unregister(&g_pdm_led_proc);
}
