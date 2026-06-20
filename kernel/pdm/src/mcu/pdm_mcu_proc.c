// SPDX-License-Identifier: GPL-2.0

#include "pdm_mcu_internal.h"
#include "pdm_debugfs.h"
#include "pdm_proc.h"
#include "pdm_status.h"

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kstrtox.h>
#include <linux/string.h>

static pdm_proc_entry_t g_pdm_mcu_proc;
static pdm_debugfs_entry_t g_pdm_mcu_debugfs;

static void pdm_mcu_proc_format_bytes(const uint8_t *data, uint32_t size,
				      char *buffer, uint32_t buffer_size)
{
	uint32_t i;
	uint32_t offset = 0;
	int ret;

	if (!buffer || buffer_size == 0)
		return;

	buffer[0] = '\0';
	for (i = 0; i < size && offset < buffer_size; i++) {
		ret = osal_snprintf(buffer + offset, buffer_size - offset,
				    "%s%02x", i == 0 ? "" : " ", data[i]);
		if (ret < 0)
			break;
		if ((uint32_t)ret >= buffer_size - offset) {
			offset = buffer_size - 1;
			break;
		}
		offset += ret;
	}
	buffer[buffer_size - 1] = '\0';
}

static char *pdm_mcu_proc_next_token(char **cursor)
{
	char *token;

	while ((token = strsep(cursor, " \t\r\n")) != NULL) {
		if (*token != '\0')
			return token;
	}

	return NULL;
}

static int pdm_mcu_proc_parse_u32(char **cursor, uint32_t *value)
{
	char *token = pdm_mcu_proc_next_token(cursor);

	if (!token)
		return -EINVAL;

	return kstrtou32(token, 0, value);
}

static int pdm_mcu_proc_parse_bytes(char **cursor, uint8_t *buffer,
				    uint32_t max_size, uint32_t *actual_size)
{
	char *token;
	uint32_t value;
	uint32_t count = 0;
	int ret;

	while ((token = pdm_mcu_proc_next_token(cursor)) != NULL) {
		if (count >= max_size)
			return -E2BIG;

		ret = kstrtou32(token, 0, &value);
		if (ret)
			return ret;
		if (value > 0xFFU)
			return -ERANGE;

		buffer[count++] = (uint8_t)value;
	}

	*actual_size = count;
	return 0;
}

static pdm_mcu_handle_t pdm_mcu_proc_get_handle(uint32_t index)
{
	if (index >= PDM_MCU_MAX_DEVICES)
		return NULL;

	return pdm_mcu_get(index);
}

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

static int pdm_mcu_proc_do_version(pdm_mcu_handle_t handle, uint32_t index)
{
	pdm_mcu_version_t version;
	int32_t ret;

	osal_memset(&version, 0, sizeof(version));
	ret = pdm_mcu_get_version(handle, &version);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	LOG_INFO("PDM_MCU",
		 "debugfs version index=%u version=%u.%u.%u.%u string=%s",
		 index, version.major, version.minor, version.patch,
		 version.build, version.version_string);
	return 0;
}

static int pdm_mcu_proc_do_status(pdm_mcu_handle_t handle, uint32_t index)
{
	pdm_mcu_status_t status;
	int32_t ret;

	osal_memset(&status, 0, sizeof(status));
	ret = pdm_mcu_get_status(handle, &status);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	LOG_INFO("PDM_MCU",
		 "debugfs status index=%u online=%u state=%u uptime=%u error=%u temp_mc=%d voltage_mv=%u timestamp_us=%llu",
		 index, status.online ? 1U : 0U, status.state,
		 status.uptime_sec, status.error_code,
		 status.temperature_milli_celsius, status.voltage_mv,
		 (unsigned long long)status.timestamp_us);
	return 0;
}

static int pdm_mcu_proc_do_reset(pdm_mcu_handle_t handle, uint32_t index)
{
	int32_t ret = pdm_mcu_reset(handle);

	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	LOG_INFO("PDM_MCU", "debugfs reset index=%u success", index);
	return 0;
}

static int pdm_mcu_proc_do_cmd(pdm_mcu_handle_t handle, uint32_t index,
			       uint32_t command, char **cursor)
{
	uint8_t request[64];
	uint8_t response[64];
	char response_hex[192];
	uint32_t request_len = 0;
	uint32_t response_len = 0;
	int32_t ret;
	int parse_ret;

	if (command > 0xFFU)
		return -ERANGE;

	parse_ret = pdm_mcu_proc_parse_bytes(cursor, request,
					     sizeof(request), &request_len);
	if (parse_ret)
		return parse_ret;

	ret = pdm_mcu_send_command(handle, (uint8_t)command, request,
				   request_len, response, sizeof(response),
				   &response_len);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	pdm_mcu_proc_format_bytes(response, response_len, response_hex,
				  sizeof(response_hex));
	LOG_INFO("PDM_MCU",
		 "debugfs cmd index=%u cmd=0x%02x tx_len=%u rx_len=%u rx=[%s]",
		 index, command, request_len, response_len, response_hex);
	return 0;
}

static int pdm_mcu_proc_do_read(pdm_mcu_handle_t handle, uint32_t index,
				uint32_t address, uint32_t size)
{
	uint8_t data[64];
	char data_hex[192];
	int32_t ret;

	if (size == 0 || size > sizeof(data))
		return -EINVAL;

	ret = pdm_mcu_read_data(handle, address, data, size);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	pdm_mcu_proc_format_bytes(data, size, data_hex, sizeof(data_hex));
	LOG_INFO("PDM_MCU",
		 "debugfs read index=%u addr=0x%08x len=%u data=[%s]",
		 index, address, size, data_hex);
	return 0;
}

static int pdm_mcu_proc_do_write(pdm_mcu_handle_t handle, uint32_t index,
				 uint32_t address, char **cursor)
{
	uint8_t data[64];
	uint32_t size = 0;
	int32_t ret;
	int parse_ret;

	parse_ret = pdm_mcu_proc_parse_bytes(cursor, data, sizeof(data), &size);
	if (parse_ret)
		return parse_ret;
	if (size == 0)
		return -EINVAL;

	ret = pdm_mcu_write_data(handle, address, data, size);
	if (ret != OSAL_SUCCESS)
		return pdm_status_to_errno(ret);

	LOG_INFO("PDM_MCU", "debugfs write index=%u addr=0x%08x len=%u",
		 index, address, size);
	return 0;
}

static int pdm_mcu_proc_write(char *command, size_t count, void *data)
{
	pdm_mcu_handle_t handle;
	char *cursor = command;
	char *op;
	uint32_t index;
	uint32_t value;
	uint32_t size;
	int ret;

	(void)count;
	(void)data;

	op = pdm_mcu_proc_next_token(&cursor);
	if (!op)
		return -EINVAL;

	ret = pdm_mcu_proc_parse_u32(&cursor, &index);
	if (ret)
		return ret;

	handle = pdm_mcu_proc_get_handle(index);
	if (!handle) {
		pdm_mcu_chrdev_record_error(index, -ENODEV);
		return -ENODEV;
	}

	if (!strcmp(op, "version")) {
		ret = pdm_mcu_proc_do_version(handle, index);
		goto out;
	}

	if (!strcmp(op, "status")) {
		ret = pdm_mcu_proc_do_status(handle, index);
		goto out;
	}

	if (!strcmp(op, "reset")) {
		ret = pdm_mcu_proc_do_reset(handle, index);
		goto out;
	}

	if (!strcmp(op, "cmd")) {
		ret = pdm_mcu_proc_parse_u32(&cursor, &value);
		if (ret)
			goto out;
		ret = pdm_mcu_proc_do_cmd(handle, index, value, &cursor);
		goto out;
	}

	if (!strcmp(op, "read")) {
		ret = pdm_mcu_proc_parse_u32(&cursor, &value);
		if (ret)
			goto out;
		ret = pdm_mcu_proc_parse_u32(&cursor, &size);
		if (ret)
			goto out;
		ret = pdm_mcu_proc_do_read(handle, index, value, size);
		goto out;
	}

	if (!strcmp(op, "write")) {
		ret = pdm_mcu_proc_parse_u32(&cursor, &value);
		if (ret)
			goto out;
		ret = pdm_mcu_proc_do_write(handle, index, value, &cursor);
		goto out;
	}

	ret = -EINVAL;

out:
	if (ret)
		pdm_mcu_chrdev_record_error(index, ret);
	return ret;
}

int pdm_mcu_proc_register(void)
{
	return pdm_proc_register(&g_pdm_mcu_proc, "mcu",
				 pdm_mcu_proc_show, NULL, NULL);
}

void pdm_mcu_proc_unregister(void)
{
	pdm_proc_unregister(&g_pdm_mcu_proc);
}

int pdm_mcu_debugfs_register(void)
{
	return pdm_debugfs_register(&g_pdm_mcu_debugfs, "mcu",
				    pdm_mcu_proc_write, NULL);
}

void pdm_mcu_debugfs_unregister(void)
{
	pdm_debugfs_unregister(&g_pdm_mcu_debugfs);
}
