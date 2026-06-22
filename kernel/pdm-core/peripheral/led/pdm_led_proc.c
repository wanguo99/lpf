// SPDX-License-Identifier: GPL-2.0

#include "lpf_led_internal.h"
#include "lpf/core/lpf_debugfs.h"
#include "lpf/lpf_errno.h"
#include "lpf/core/lpf_proc.h"

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kstrtox.h>
#include <linux/string.h>

static lpf_proc_entry_t g_lpf_led_proc;
static lpf_debugfs_entry_t g_lpf_led_debugfs;

static char *lpf_led_proc_next_token(char **cursor)
{
	char *token;

	while ((token = strsep(cursor, " \t\r\n")) != NULL) {
		if (*token != '\0')
			return token;
	}

	return NULL;
}

static int lpf_led_proc_parse_u32(char **cursor, uint32_t *value)
{
	char *token = lpf_led_proc_next_token(cursor);

	if (!token)
		return -EINVAL;

	return kstrtou32(token, 0, value);
}

static lpf_led_handle_t lpf_led_proc_get_handle(uint32_t index)
{
	if (index >= LPF_LED_MAX_DEVICES)
		return NULL;

	return lpf_led_get(index);
}

static const char *lpf_led_control_name(lpf_config_led_control_t control)
{
	switch (control) {
	case LPF_CONFIG_LED_CONTROL_GPIO:
		return "gpio";
	case LPF_CONFIG_LED_CONTROL_PWM:
		return "pwm";
	default:
		return "unknown";
	}
}

static int lpf_led_proc_show(struct seq_file *seq, void *data)
{
	lpf_led_debug_info_t info;
	uint32_t i;
	int32_t ret;

	(void)data;

	seq_puts(seq, "LPF LED\n");
	seq_printf(seq, "max_devices=%u\n", LPF_LED_MAX_DEVICES);
	seq_puts(seq, "devices:\n");

	for (i = 0; i < LPF_LED_MAX_DEVICES; i++) {
		ret = lpf_led_debug_get(i, &info);
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
			   i, info.name, lpf_led_control_name(info.control),
			   info.enabled ? 1U : 0U, info.brightness,
			   info.max_brightness);
	}

	return 0;
}

static int lpf_led_proc_do_state(lpf_led_handle_t handle, uint32_t index)
{
	lpf_led_service_state_t state;
	int32_t ret;

	osal_memset(&state, 0, sizeof(state));
	ret = lpf_led_get_state(handle, &state);
	if (ret != OSAL_SUCCESS)
		return lpf_status_to_errno(ret);

	LOG_INFO("LPF_LED",
		 "debugfs state index=%u enabled=%u brightness=%u max_brightness=%u",
		 index, state.enabled ? 1U : 0U, state.brightness,
		 state.max_brightness);
	return 0;
}

static int lpf_led_proc_write(char *command, size_t count, void *data)
{
	lpf_led_handle_t handle;
	char *cursor = command;
	char *op;
	uint32_t index;
	uint32_t brightness;
	int32_t status;
	int ret;

	(void)count;
	(void)data;

	op = lpf_led_proc_next_token(&cursor);
	if (!op)
		return -EINVAL;

	ret = lpf_led_proc_parse_u32(&cursor, &index);
	if (ret)
		return ret;

	handle = lpf_led_proc_get_handle(index);
	if (!handle) {
		lpf_led_chrdev_record_error(index, -ENODEV);
		return -ENODEV;
	}

	if (!strcmp(op, "state")) {
		ret = lpf_led_proc_do_state(handle, index);
		goto out;
	}

	if (!strcmp(op, "enable") || !strcmp(op, "on")) {
		status = lpf_led_enable(handle);
		if (status != OSAL_SUCCESS) {
			ret = lpf_status_to_errno(status);
			goto out;
		}
		LOG_INFO("LPF_LED", "debugfs enable index=%u success", index);
		ret = 0;
		goto out;
	}

	if (!strcmp(op, "disable") || !strcmp(op, "off")) {
		status = lpf_led_disable(handle);
		if (status != OSAL_SUCCESS) {
			ret = lpf_status_to_errno(status);
			goto out;
		}
		LOG_INFO("LPF_LED", "debugfs disable index=%u success", index);
		ret = 0;
		goto out;
	}

	if (!strcmp(op, "set") || !strcmp(op, "brightness")) {
		ret = lpf_led_proc_parse_u32(&cursor, &brightness);
		if (ret)
			goto out;

		status = lpf_led_set_brightness(handle, brightness);
		if (status != OSAL_SUCCESS) {
			ret = lpf_status_to_errno(status);
			goto out;
		}
		LOG_INFO("LPF_LED",
			 "debugfs set index=%u brightness=%u success",
			 index, brightness);
		ret = 0;
		goto out;
	}

	ret = -EINVAL;

out:
	if (ret == 0)
		lpf_led_chrdev_record_recovery(index);
	else if (lpf_errno_is_runtime_error(ret))
		lpf_led_chrdev_record_error(index, ret);
	return ret;
}

int lpf_led_proc_register(void)
{
	return lpf_proc_register(&g_lpf_led_proc, "led",
				 lpf_led_proc_show, NULL, NULL);
}

void lpf_led_proc_unregister(void)
{
	lpf_proc_unregister(&g_lpf_led_proc);
}

int lpf_led_debugfs_register(void)
{
	return lpf_debugfs_register(&g_lpf_led_debugfs, "lpf", "led",
				    lpf_led_proc_write, NULL);
}

void lpf_led_debugfs_unregister(void)
{
	lpf_debugfs_unregister(&g_lpf_led_debugfs);
}
