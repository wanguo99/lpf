// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_diag.c
 * @brief PDM MCU diagnostic interfaces (proc and debugfs)
 */

#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/seq_file.h>
#include <linux/string.h>

#include "pdm_mcu_internal.h"
#include "pdm/diag/pdm_proc.h"
#include "pdm/diag/pdm_debugfs.h"
#include "pdm/bus/pdm_bus.h"
#include "pdm/pdm_mcu.h"
#include "pdm/log/pdm_log.h"

static pdm_proc_entry_t g_mcu_proc;
static pdm_debugfs_entry_t g_mcu_debugfs;
static atomic_t *g_device_count;

#define PDM_MCU_DIAG_LOOP_DEFAULT_COUNT 1000U
#define PDM_MCU_DIAG_LOOP_MAX_COUNT 1000000U
#define PDM_MCU_DIAG_LOOP_DEFAULT_DELAY_US 1000U
#define PDM_MCU_DIAG_LOOP_MAX_DELAY_US 1000000U
#define PDM_MCU_DIAG_LOOP_DEFAULT_LEN 8U
#define PDM_MCU_DIAG_CAN_MAX_DLEN 8U

static void pdm_mcu_diag_fill_pattern(u8 *buf, u32 len, u32 iter)
{
	u32 i;

	for (i = 0; i < len; i++) {
		buf[i] = (u8)(iter + i);
	}
}

static void pdm_mcu_diag_loop_delay(u32 delay_us)
{
	if (!delay_us) {
		cond_resched();
		return;
	}
	if (delay_us < 1000U) {
		usleep_range(delay_us, delay_us + 50U);
		return;
	}

	msleep(DIV_ROUND_UP(delay_us, 1000U));
}

static int pdm_mcu_diag_loop_xfer(struct pdm_mcu_instance *inst, u32 index,
				  bool read_loop, u32 command_id,
				  u32 count, u32 delay_us, u32 data_len)
{
	struct pdm_mcu_xfer xfer;
	u8 tx_data[PDM_MCU_MAX_TRANSFER_SIZE];
	u8 rx_data[PDM_MCU_MAX_TRANSFER_SIZE];
	bool continuous = count == 0;
	u64 i;
	int ret;

	if (!inst->ops || !inst->ops->xfer) {
		return -EOPNOTSUPP;
	}
	if (count > PDM_MCU_DIAG_LOOP_MAX_COUNT ||
	    delay_us > PDM_MCU_DIAG_LOOP_MAX_DELAY_US) {
		return -EINVAL;
	}
	if (data_len > PDM_MCU_MAX_TRANSFER_SIZE) {
		return -EMSGSIZE;
	}
	if (read_loop && !data_len) {
		return -EINVAL;
	}

	if (continuous) {
		LOG_INFO("MCU %u %s loop start: cmd=0x%x count=continuous delay_us=%u len=%u transport=%s",
			 index, read_loop ? "read" : "write", command_id,
			 delay_us, data_len, inst->ops->name);
	} else {
		LOG_INFO("MCU %u %s loop start: cmd=0x%x count=%u delay_us=%u len=%u transport=%s",
			 index, read_loop ? "read" : "write", command_id,
			 count, delay_us, data_len, inst->ops->name);
	}

	for (i = 0; continuous || i < count; i++) {
		if (signal_pending(current)) {
			LOG_INFO("MCU %u %s loop interrupted at %llu transfers",
				 index, read_loop ? "read" : "write", i);
			return -EINTR;
		}

		memset(&xfer, 0, sizeof(xfer));
		xfer.command = command_id;

		if (read_loop) {
			xfer.rx = rx_data;
			xfer.rx_len = data_len;
		} else {
			pdm_mcu_diag_fill_pattern(tx_data, data_len, i);
			xfer.tx = tx_data;
			xfer.tx_len = data_len;
		}

		ret = inst->ops->xfer(inst, &xfer);
		if (ret) {
			if (continuous) {
				LOG_ERROR("MCU %u %s loop failed at %llu: %d",
					  index, read_loop ? "read" : "write",
					  i + 1, ret);
			} else {
				LOG_ERROR("MCU %u %s loop failed at %llu/%u: %d",
					  index, read_loop ? "read" : "write",
					  i + 1, count, ret);
			}
			return ret;
		}

		if (continuous) {
			LOG_DEBUG("MCU %u %s loop iter=%llu cmd=0x%x tx=%u rx=%u actual_rx=%u",
				  index, read_loop ? "read" : "write", i + 1,
				  command_id, xfer.tx_len, xfer.rx_len,
				  xfer.actual_rx_len);
		} else {
			LOG_DEBUG("MCU %u %s loop iter=%llu/%u cmd=0x%x tx=%u rx=%u actual_rx=%u",
				  index, read_loop ? "read" : "write", i + 1, count,
				  command_id, xfer.tx_len, xfer.rx_len,
				  xfer.actual_rx_len);
		}
		pdm_mcu_diag_loop_delay(delay_us);
	}

	LOG_INFO("MCU %u %s loop complete: count=%u", index,
		 read_loop ? "read" : "write", count);
	return 0;
}

static int pdm_mcu_diag_can_write_loop(struct pdm_mcu_instance *inst,
				       u32 index, u32 can_id, u32 count,
				       u32 delay_us, u32 data_len)
{
	u8 data[PDM_MCU_DIAG_CAN_MAX_DLEN];
	bool continuous = count == 0;
	u64 i;
	int ret;

	if (!inst->ops ||
	    !(inst->ops->capability & PDM_MCU_CAP_TRANSPORT_CAN)) {
		return -EOPNOTSUPP;
	}
	if (count > PDM_MCU_DIAG_LOOP_MAX_COUNT ||
	    delay_us > PDM_MCU_DIAG_LOOP_MAX_DELAY_US ||
	    data_len > PDM_MCU_DIAG_CAN_MAX_DLEN) {
		return -EINVAL;
	}

	if (continuous) {
		LOG_INFO("MCU %u CAN std loop start: id=0x%x count=continuous delay_us=%u dlc=%u",
			 index, can_id, delay_us, data_len);
	} else {
		LOG_INFO("MCU %u CAN std loop start: id=0x%x count=%u delay_us=%u dlc=%u",
			 index, can_id, count, delay_us, data_len);
	}

	for (i = 0; continuous || i < count; i++) {
		if (signal_pending(current)) {
			LOG_INFO("MCU %u CAN std loop interrupted at %llu frames",
				 index, i);
			return -EINTR;
		}

		pdm_mcu_diag_fill_pattern(data, data_len, i);
		ret = pdm_mcu_can_diag_send_std_frame(inst, can_id, data,
						      data_len);
		if (ret) {
			if (continuous) {
				LOG_ERROR("MCU %u CAN std loop failed at %llu: %d",
					  index, i + 1, ret);
			} else {
				LOG_ERROR("MCU %u CAN std loop failed at %llu/%u: %d",
					  index, i + 1, count, ret);
			}
			return ret;
		}

		if (continuous) {
			LOG_DEBUG("MCU %u CAN std loop iter=%llu id=0x%x dlc=%u",
				  index, i + 1, can_id, data_len);
		} else {
			LOG_DEBUG("MCU %u CAN std loop iter=%llu/%u id=0x%x dlc=%u",
				  index, i + 1, count, can_id, data_len);
		}
		pdm_mcu_diag_loop_delay(delay_us);
	}

	LOG_INFO("MCU %u CAN std loop complete: count=%u", index, count);
	return 0;
}

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

/* Helper to match device by index during iteration */
struct mcu_find_ctx {
	u32 target_index;
	u32 current_index;
	struct pdm_mcu_instance *result;
};

static int pdm_mcu_match_by_index(struct device *dev, void *data)
{
	struct mcu_find_ctx *ctx = data;
	struct pdm_device *pdm_dev;
	struct pdm_mcu_instance *inst;

	pdm_dev = dev_to_pdm_device(dev);
	inst = pdm_device_get_drvdata(pdm_dev);

	if (ctx->current_index == ctx->target_index) {
		ctx->result = inst;
		return 1; /* Stop iteration */
	}

	ctx->current_index++;
	return 0; /* Continue iteration */
}

/* Helper function to find MCU instance by index */
static struct pdm_mcu_instance *pdm_mcu_find_instance(u32 index)
{
	struct device_driver *drv;
	struct mcu_find_ctx ctx = {
		.target_index = index,
		.current_index = 0,
		.result = NULL,
	};
	int ret;

	drv = driver_find("pdm-mcu", &pdm_bus_type);
	if (!drv) {
		return NULL;
	}

	ret = driver_for_each_device(drv, NULL, &ctx, pdm_mcu_match_by_index);
	if (ret < 0) {
		return NULL;
	}

	return ctx.result;
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

		ret = pdm_cdev_instance_claim(&inst->base);
		if (ret != 0) {
			seq_printf(seq, "  %2u  <busy>\n", i);
			continue;
		}

		/* Get MCU status */
		memset(&status, 0, sizeof(status));
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

		pdm_cdev_instance_release(&inst->base);
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
		LOG_INFO("  can_write_loop <index> <can_id_hex> [count] [delay_us] [dlc]");
		LOG_INFO("                   - Repeated classic standard CAN data frames");
		LOG_INFO("  write_loop <index> <cmd_hex> [count] [delay_us] [tx_len]");
		LOG_INFO("                   - Repeated write-only transfers; count 0 runs until interrupted");
		LOG_INFO("  read_loop <index> <cmd_hex> [count] [delay_us] [rx_len]");
		LOG_INFO("                   - Repeated command plus read transfers; count 0 runs until interrupted");
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

	ret = pdm_cdev_instance_claim(&inst->base);
	if (ret) {
		LOG_ERROR("MCU device %u is busy", index);
		return ret;
	}

	if (strcmp(cmd, "version") == 0) {
		memset(&version, 0, sizeof(version));
		ret = pdm_mcu_protocol_get_version(inst, &version);
		if (ret == 0) {
			LOG_INFO("MCU %u version: %u.%u.%u.%u (%s)",
				 index, version.major, version.minor,
				 version.patch, version.build,
				 version.version_string);
		}
	} else if (strcmp(cmd, "status") == 0) {
		memset(&status, 0, sizeof(status));
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
		ret = pdm_mcu_protocol_reset(inst);
		if (ret == 0) {
			LOG_INFO("MCU %u reset successful", index);
		}
	} else if (strcmp(cmd, "can_write_loop") == 0) {
		u32 can_id;
		u32 loop_count = PDM_MCU_DIAG_LOOP_DEFAULT_COUNT;
		u32 delay_us = PDM_MCU_DIAG_LOOP_DEFAULT_DELAY_US;
		u32 data_len = PDM_MCU_DIAG_CAN_MAX_DLEN;

		n = sscanf(command, "%31s %u %x %u %u %u", cmd, &index,
			   &can_id, &loop_count, &delay_us, &data_len);
		if (n < 3) {
			LOG_ERROR("Usage: can_write_loop <index> <can_id_hex> [count] [delay_us] [dlc]");
			ret = -EINVAL;
		} else {
			ret = pdm_mcu_diag_can_write_loop(inst, index, can_id,
						      loop_count, delay_us,
						      data_len);
		}
	} else if (strcmp(cmd, "write_loop") == 0 ||
		   strcmp(cmd, "read_loop") == 0) {
		bool read_loop = strcmp(cmd, "read_loop") == 0;
		u32 command_id;
		u32 loop_count = PDM_MCU_DIAG_LOOP_DEFAULT_COUNT;
		u32 delay_us = PDM_MCU_DIAG_LOOP_DEFAULT_DELAY_US;
		u32 data_len = PDM_MCU_DIAG_LOOP_DEFAULT_LEN;

		n = sscanf(command, "%31s %u %x %u %u %u", cmd, &index,
			   &command_id, &loop_count, &delay_us, &data_len);
		if (n < 3) {
			LOG_ERROR("Usage: %s <index> <cmd_hex> [count] [delay_us] [len]",
				  read_loop ? "read_loop" : "write_loop");
			ret = -EINVAL;
		} else {
			ret = pdm_mcu_diag_loop_xfer(inst, index, read_loop,
							command_id, loop_count,
							delay_us, data_len);
		}
	} else {
		LOG_ERROR("Unknown command '%s'. Use 'help' for usage.", cmd);
		ret = -EINVAL;
	}

	pdm_cdev_instance_release(&inst->base);
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

	ret = pdm_cdev_instance_claim(&inst->base);
	if (ret) {
		return ret;
	}

	if (strcmp(cmd, "reset") == 0) {
		ret = pdm_mcu_protocol_reset(inst);
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
				char hex_buf[32 * 3 + 1];
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
	pdm_cdev_instance_release(&inst->base);
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
