// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_diag.c
 * @brief PDM MCU diagnostic interfaces (proc and debugfs)
 */

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/string.h>

#include "pdm_mcu_internal.h"
#include "pdm/diag/pdm_proc.h"
#include "pdm/diag/pdm_debugfs.h"
#include "pdm/bus/pdm_bus.h"
#include "pdm/pdm_mcu.h"
#include "osal.h"

static pdm_proc_entry_t g_mcu_proc;
static pdm_debugfs_entry_t g_mcu_debugfs;
static atomic_t *g_device_count;

static const char *pdm_mcu_state_str(u32 state)
{
	switch (state) {
	case PDM_MCU_STATE_UNINITIALIZED:
		return "UNINITIALIZED";
	case PDM_MCU_STATE_INIT:
		return "INIT";
	case PDM_MCU_STATE_READY:
		return "READY";
	case PDM_MCU_STATE_BUSY:
		return "BUSY";
	case PDM_MCU_STATE_ERROR:
		return "ERROR";
	case PDM_MCU_STATE_OFFLINE:
		return "OFFLINE";
	default:
		return "UNKNOWN";
	}
}

/* Helper function to find MCU instance by index */
static struct pdm_mcu_instance *pdm_mcu_find_instance(u32 index)
{
	struct pdm_device *pdm_dev;
	struct device *dev;
	struct device_driver *drv;
	char devname[32];

	drv = driver_find("pdm-mcu", &pdm_bus_type);
	if (!drv) {
		return NULL;
	}

	snprintf(devname, sizeof(devname), "pdm-mcu.%u", index);
	dev = driver_find_device_by_name(drv, devname);
	if (!dev) {
		return NULL;
	}

	pdm_dev = dev_to_pdm_device(dev);
	put_device(dev);
	return pdm_device_get_drvdata(pdm_dev);
}

/* Proc show function */
static int pdm_mcu_proc_show(struct seq_file *seq, void *data)
{
	struct pdm_mcu_instance *inst;
	struct pdm_mcu_status status;
	u32 count;
	u32 i;
	int ret;

	(void)data;

	seq_printf(seq, "PDM MCU Module\n");
	seq_printf(seq, "ABI Version:    0x%08x\n", PDM_MCU_ABI_VERSION);
	seq_printf(seq, "Module Version: 1.0.0\n");

	count = atomic_read(g_device_count);
	seq_printf(seq, "Device Count:   %u\n", count);
	seq_printf(seq, "\n");

	if (count == 0) {
		seq_printf(seq, "No MCU devices registered\n");
		return 0;
	}

	seq_printf(seq, "MCU Devices:\n");
	seq_printf(seq, "  ID  State       Online  Uptime    Temp      Voltage  Transport\n");
	seq_printf(seq, "  --  ----------  ------  --------  --------  -------  ---------\n");

	for (i = 0; i < count; i++) {
		inst = pdm_mcu_find_instance(i);
		if (!inst) {
			seq_printf(seq, "  %2u  <not found>\n", i);
			continue;
		}

		ret = pdm_driver_claim(&inst->base);
		if (ret != 0) {
			seq_printf(seq, "  %2u  <busy>\n", i);
			continue;
		}

		/* Get MCU status */
		status.index = i;
		ret = pdm_mcu_protocol_get_status(inst, &status);

		if (ret == 0) {
			s32 temp_c = status.temperature_milli_celsius / 1000;
			s32 temp_frac = abs(status.temperature_milli_celsius % 1000) / 100;

			seq_printf(seq, "  %2u  %-10s  %-6s  %5us     %3d.%d°C   %4umV   %s\n",
				   i,
				   pdm_mcu_state_str(status.state),
				   status.online ? "yes" : "no",
				   status.uptime_sec,
				   temp_c, temp_frac,
				   status.voltage_mv,
				   inst->ops ? inst->ops->name : "none");
		} else {
			seq_printf(seq, "  %2u  %-10s  %-6s  ---       ---       ---      %s\n",
				   i,
				   pdm_mcu_state_str(inst->state),
				   inst->base.online ? "yes" : "no",
				   inst->ops ? inst->ops->name : "none");
		}

		pdm_driver_release(&inst->base);
	}

	seq_printf(seq, "\n");
	seq_printf(seq, "Usage:\n");
	seq_printf(seq, "  cat /proc/pdm/mcu          - Show this information\n");
	seq_printf(seq, "  echo \"help\" > /proc/pdm/mcu - Show available commands\n");

	return 0;
}

/* Proc write function */
static int pdm_mcu_proc_write(char *command, size_t count, void *data)
{
	struct pdm_mcu_instance *inst;
	struct pdm_mcu_version version;
	struct pdm_mcu_status status;
	char cmd[32];
	u32 index;
	int ret;
	int n;

	(void)data;

	/* Remove trailing newline */
	if (count > 0 && command[count - 1] == '\n') {
		command[count - 1] = '\0';
		count--;
	}

	if (strncmp(command, "help", 4) == 0) {
		LOG_INFO("Available commands:");
		LOG_INFO("  version <index>  - Get MCU firmware version");
		LOG_INFO("  status <index>   - Get MCU status");
		LOG_INFO("  reset <index>    - Reset MCU");
		LOG_INFO("  help             - Show this help");
		return 0;
	}

	n = sscanf(command, "%31s %u", cmd, &index);
	if (n < 2) {
		LOG_ERROR("Invalid command format. Use 'help' for usage.");
		return -EINVAL;
	}

	inst = pdm_mcu_find_instance(index);
	if (!inst) {
		LOG_ERROR("MCU device %u not found", index);
		return -ENODEV;
	}

	ret = pdm_driver_claim(&inst->base);
	if (ret) {
		LOG_ERROR("MCU device %u is busy", index);
		return ret;
	}

	if (strcmp(cmd, "version") == 0) {
		version.index = index;
		ret = pdm_mcu_protocol_get_version(inst, &version);
		if (ret == 0) {
			LOG_INFO("MCU %u version: %u.%u.%u.%u (%s)",
				 index, version.major, version.minor,
				 version.patch, version.build,
				 version.version_string);
		}
	} else if (strcmp(cmd, "status") == 0) {
		status.index = index;
		ret = pdm_mcu_protocol_get_status(inst, &status);
		if (ret == 0) {
			LOG_INFO("MCU %u: state=%s online=%s uptime=%us temp=%d.%d°C voltage=%umV",
				 index,
				 pdm_mcu_state_str(status.state),
				 status.online ? "yes" : "no",
				 status.uptime_sec,
				 status.temperature_milli_celsius / 1000,
				 abs(status.temperature_milli_celsius % 1000) / 100,
				 status.voltage_mv);
		}
	} else if (strcmp(cmd, "reset") == 0) {
		ret = pdm_mcu_protocol_reset(inst, index);
		if (ret == 0) {
			LOG_INFO("MCU %u reset successful", index);
		}
	} else {
		LOG_ERROR("Unknown command '%s'. Use 'help' for usage.", cmd);
		ret = -EINVAL;
	}

	pdm_driver_release(&inst->base);
	return ret;
}

/* Debugfs write function */
static int pdm_mcu_debugfs_write(char *command, size_t count, void *data)
{
	struct pdm_mcu_instance *inst;
	struct pdm_mcu_command mcu_cmd;
	char cmd[32];
	u32 index;
	u32 command_id;
	u8 tx_data[16];
	int tx_len = 0;
	int ret;
	int n;
	int i;

	(void)data;

	/* Remove trailing newline */
	if (count > 0 && command[count - 1] == '\n') {
		command[count - 1] = '\0';
		count--;
	}

	/* Parse command: "cmd <index> [args...]" */
	n = sscanf(command, "%31s %u", cmd, &index);
	if (n < 2) {
		LOG_ERROR("Invalid command format");
		return -EINVAL;
	}

	inst = pdm_mcu_find_instance(index);
	if (!inst) {
		LOG_ERROR("MCU device %u not found", index);
		return -ENODEV;
	}

	ret = pdm_driver_claim(&inst->base);
	if (ret) {
		return ret;
	}

	if (strcmp(cmd, "reset") == 0) {
		ret = pdm_mcu_protocol_reset(inst, index);
	} else if (strcmp(cmd, "command") == 0) {
		/* Parse: "command <index> <cmd_id> [hex bytes...]" */
		char *p = command;
		/* Skip "command" and index */
		p = strchr(p, ' ');
		if (p) p = strchr(p + 1, ' ');
		if (!p) {
			ret = -EINVAL;
			goto unlock;
		}

		if (sscanf(p, "%u", &command_id) != 1) {
			ret = -EINVAL;
			goto unlock;
		}

		/* Parse hex data bytes */
		p = strchr(p, ' ');
		while (p && tx_len < 16) {
			u32 byte;
			if (sscanf(p, "%x", &byte) == 1) {
				tx_data[tx_len++] = (u8)byte;
				p = strchr(p + 1, ' ');
			} else {
				break;
			}
		}

		memset(&mcu_cmd, 0, sizeof(mcu_cmd));
		mcu_cmd.index = index;
		mcu_cmd.command = command_id;
		mcu_cmd.flags = PDM_MCU_CMD_F_NEED_RESPONSE;
		mcu_cmd.tx_len = tx_len;
		memcpy(mcu_cmd.tx_data, tx_data, tx_len);
		mcu_cmd.rx_len = PDM_MCU_MAX_TRANSFER_SIZE;

		ret = pdm_mcu_protocol_command(inst, &mcu_cmd);

		if (ret == 0) {
			LOG_INFO("MCU %u command 0x%x: sent %u bytes, received %u bytes",
				 index, command_id, tx_len, mcu_cmd.actual_rx_len);
			if (mcu_cmd.actual_rx_len > 0) {
				char hex_buf[PDM_MCU_MAX_TRANSFER_SIZE * 3 + 1];
				char *p_hex = hex_buf;
				for (i = 0; i < mcu_cmd.actual_rx_len && i < 32; i++) {
					p_hex += sprintf(p_hex, "%02x ", mcu_cmd.rx_data[i]);
				}
				LOG_INFO("  Response: %s", hex_buf);
			}
		}
	} else {
		ret = -EINVAL;
	}

unlock:
	pdm_driver_release(&inst->base);
	return ret;
}

int pdm_mcu_diag_init(atomic_t *device_count)
{
	int ret;

	g_device_count = device_count;

	/* Register proc interface */
	ret = pdm_proc_register(&g_mcu_proc, "mcu",
				pdm_mcu_proc_show,
				pdm_mcu_proc_write,
				NULL);
	if (ret) {
		LOG_ERROR("Failed to register proc interface: %d", ret);
		return ret;
	}

	/* Register debugfs interface */
	ret = pdm_debugfs_register(&g_mcu_debugfs, "pdm", "mcu_control",
				   pdm_mcu_debugfs_write, NULL);
	if (ret) {
		LOG_WARN("Failed to register debugfs interface: %d", ret);
		/* Non-fatal, proc interface is still available */
	}

	return 0;
}

void pdm_mcu_diag_exit(void)
{
	pdm_debugfs_unregister(&g_mcu_debugfs);
	pdm_proc_unregister(&g_mcu_proc);
}
