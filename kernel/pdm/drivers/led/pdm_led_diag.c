// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_led_diag.c
 * @brief PDM LED diagnostic interfaces (proc and debugfs)
 */

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/string.h>

#include "pdm_led_internal.h"
#include "pdm/diag/pdm_proc.h"
#include "pdm/diag/pdm_debugfs.h"
#include "pdm/bus/pdm_bus.h"
#include "pdm/pdm_led.h"
#include "pdm/log/pdm_log.h"

static pdm_proc_entry_t g_led_proc;
static pdm_debugfs_entry_t g_led_debugfs;
static atomic_t *g_device_count;

/* Helper to match device by index during iteration */
struct led_find_ctx {
	u32 target_index;
	u32 current_index;
	struct pdm_led_instance *result;
};

static int pdm_led_match_by_index(struct device *dev, void *data)
{
	struct led_find_ctx *ctx = data;
	struct pdm_device *pdm_dev;
	struct pdm_led_instance *inst;

	pdm_dev = dev_to_pdm_device(dev);
	inst = pdm_device_get_drvdata(pdm_dev);

	if (ctx->current_index == ctx->target_index) {
		ctx->result = inst;
		return 1; /* Stop iteration */
	}

	ctx->current_index++;
	return 0; /* Continue iteration */
}

/* Helper function to find LED instance by index */
static struct pdm_led_instance *pdm_led_find_instance(u32 index)
{
	struct device_driver *drv;
	struct led_find_ctx ctx = {
		.target_index = index,
		.current_index = 0,
		.result = NULL,
	};
	int ret;

	drv = driver_find("pdm-led", &pdm_bus_type);
	if (!drv) {
		return NULL;
	}

	ret = driver_for_each_device(drv, NULL, &ctx, pdm_led_match_by_index);
	if (ret < 0) {
		return NULL;
	}

	return ctx.result;
}

/* Proc show function */
static int pdm_led_proc_show(struct seq_file *seq, void *data)
{
	struct pdm_led_instance *inst;
	u32 count;
	u32 i;

	(void)data;

	seq_printf(seq, "PDM LED Module\n");
	seq_printf(seq, "ABI Version:    0x%08x\n", PDM_LED_ABI_VERSION);
	seq_printf(seq, "Module Version: 1.0.0\n");

	count = atomic_read(g_device_count);
	seq_printf(seq, "Device Count:   %u\n", count);
	seq_printf(seq, "\n");

	if (count == 0) {
		seq_printf(seq, "No LED devices registered\n");
		return 0;
	}

	seq_printf(seq, "LED Devices:\n");
	seq_printf(seq, "  ID  Brightness  Max  Enabled  Backend\n");
	seq_printf(seq, "  --  ----------  ---  -------  -------\n");

	for (i = 0; i < count; i++) {
		inst = pdm_led_find_instance(i);
		if (!inst) {
			seq_printf(seq, "  %2u  <not found>\n", i);
			continue;
		}

		if (pdm_cdev_instance_claim(&inst->base) == 0) {
			seq_printf(seq, "  %2u  %10u  %3u  %7s  %s\n",
				   i,
				   inst->brightness,
				   inst->max_brightness,
				   inst->enabled ? "yes" : "no",
				   inst->ops ? inst->ops->name : "none");
			pdm_cdev_instance_release(&inst->base);
		} else {
			seq_printf(seq, "  %2u  <busy>\n", i);
		}
	}

	seq_printf(seq, "\n");
	seq_printf(seq, "Usage:\n");
	seq_printf(seq, "  cat /proc/pdm/led          - Show this information\n");
	seq_printf(seq, "  echo \"help\" > /proc/pdm/led - Show available commands\n");

	return 0;
}

/* Proc write function */
static int pdm_led_proc_write(char *command, size_t count, void *data)
{
	struct pdm_led_instance *inst;
	char cmd[32];
	u32 index;
	u32 value;
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
		LOG_INFO("  get <index>               - Get LED state");
		LOG_INFO("  set <index> <brightness>  - Set brightness (0-%u)",
			 PDM_LED_DEFAULT_MAX_BRIGHTNESS);
		LOG_INFO("  enable <index>            - Enable LED");
		LOG_INFO("  disable <index>           - Disable LED");
		LOG_INFO("  help                      - Show this help");
		return 0;
	}

	n = sscanf(command, "%31s %u %u", cmd, &index, &value);
	if (n < 2) {
		LOG_ERROR("Invalid command format. Use 'help' for usage.");
		return -EINVAL;
	}

	inst = pdm_led_find_instance(index);
	if (!inst) {
		LOG_ERROR("LED device %u not found", index);
		return -ENODEV;
	}

	ret = pdm_cdev_instance_claim(&inst->base);
	if (ret) {
		LOG_ERROR("LED device %u is busy", index);
		return ret;
	}

	if (strcmp(cmd, "get") == 0) {
		LOG_INFO("LED %u: brightness=%u/%u enabled=%s",
			 index, inst->brightness, inst->max_brightness,
			 inst->enabled ? "yes" : "no");
		ret = 0;
	} else if (strcmp(cmd, "set") == 0) {
		if (n < 3) {
			LOG_ERROR("Missing brightness value");
			ret = -EINVAL;
			goto unlock;
		}
		if (value > inst->max_brightness) {
			LOG_ERROR("Brightness %u exceeds maximum %u",
				value, inst->max_brightness);
			ret = -ERANGE;
			goto unlock;
		}
		inst->brightness = value;
		if (value > 0) {
			inst->enabled = 1;
		}
		ret = inst->ops->apply(inst);
		if (ret == 0) {
			LOG_INFO("LED %u brightness set to %u", index, value);
		}
	} else if (strcmp(cmd, "enable") == 0) {
		inst->enabled = 1;
		ret = inst->ops->apply(inst);
		if (ret == 0) {
			LOG_INFO("LED %u enabled", index);
		}
	} else if (strcmp(cmd, "disable") == 0) {
		inst->enabled = 0;
		ret = inst->ops->apply(inst);
		if (ret == 0) {
			LOG_INFO("LED %u disabled", index);
		}
	} else {
		LOG_ERROR("Unknown command '%s'. Use 'help' for usage.", cmd);
		ret = -EINVAL;
	}

unlock:
	pdm_cdev_instance_release(&inst->base);
	return ret;
}

/* Debugfs write function */
static int pdm_led_debugfs_write(char *command, size_t count, void *data)
{
	struct pdm_led_instance *inst;
	char cmd[32];
	u32 index;
	u32 value;
	int ret;
	int n;

	(void)data;

	/* Remove trailing newline */
	if (count > 0 && command[count - 1] == '\n') {
		command[count - 1] = '\0';
		count--;
	}

	n = sscanf(command, "%31s %u %u", cmd, &index, &value);
	if (n < 2) {
		LOG_ERROR("Invalid command format");
		return -EINVAL;
	}

	inst = pdm_led_find_instance(index);
	if (!inst) {
		LOG_ERROR("LED device %u not found", index);
		return -ENODEV;
	}

	ret = pdm_cdev_instance_claim(&inst->base);
	if (ret) {
		return ret;
	}

	if (strcmp(cmd, "brightness") == 0) {
		if (n < 3) {
			ret = -EINVAL;
			goto unlock;
		}
		if (value > inst->max_brightness) {
			ret = -ERANGE;
			goto unlock;
		}
		inst->brightness = value;
		if (value > 0) {
			inst->enabled = 1;
		}
		ret = inst->ops->apply(inst);
	} else if (strcmp(cmd, "enable") == 0) {
		inst->enabled = 1;
		ret = inst->ops->apply(inst);
	} else if (strcmp(cmd, "disable") == 0) {
		inst->enabled = 0;
		ret = inst->ops->apply(inst);
	} else if (strcmp(cmd, "toggle") == 0) {
		inst->enabled = !inst->enabled;
		ret = inst->ops->apply(inst);
	} else {
		ret = -EINVAL;
	}

unlock:
	pdm_cdev_instance_release(&inst->base);
	return ret;
}

int pdm_led_diag_init(atomic_t *device_count)
{
	int ret;

	g_device_count = device_count;

	/* Register proc interface */
	ret = pdm_proc_register(&g_led_proc, "led",
				pdm_led_proc_show,
				pdm_led_proc_write,
				NULL);
	if (ret) {
		LOG_ERROR("Failed to register proc interface: %d", ret);
		return ret;
	}

	/* Register debugfs interface */
	ret = pdm_debugfs_register(&g_led_debugfs, "pdm", "led_control",
				   pdm_led_debugfs_write, NULL);
	if (ret) {
		LOG_WARN("Failed to register debugfs interface: %d", ret);
		/* Non-fatal, proc interface is still available */
	}

	return 0;
}

void pdm_led_diag_exit(void)
{
	pdm_debugfs_unregister(&g_led_debugfs);
	pdm_proc_unregister(&g_led_proc);
}
