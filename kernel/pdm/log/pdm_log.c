// SPDX-License-Identifier: GPL-2.0

#include "pdm/log/pdm_log.h"

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>

static atomic64_t g_log_total = ATOMIC64_INIT(0);
static atomic64_t g_log_dropped = ATOMIC64_INIT(0);
static int g_log_level = PDM_LOG_DEFAULT_LEVEL;

static const char *pdm_log_level_name(int level)
{
	switch (level) {
	case PDM_LOG_LEVEL_DEBUG:
		return "DEBUG";
	case PDM_LOG_LEVEL_INFO:
		return "INFO";
	case PDM_LOG_LEVEL_WARN:
		return "WARN";
	case PDM_LOG_LEVEL_ERROR:
		return "ERROR";
	case PDM_LOG_LEVEL_FATAL:
		return "FATAL";
	default:
		return "UNKNOWN";
	}
}

static const char *pdm_log_level_prefix(int level)
{
	switch (level) {
	case PDM_LOG_LEVEL_DEBUG:
		return KERN_DEBUG;
	case PDM_LOG_LEVEL_INFO:
		return KERN_INFO;
	case PDM_LOG_LEVEL_WARN:
		return KERN_WARNING;
	case PDM_LOG_LEVEL_ERROR:
	case PDM_LOG_LEVEL_FATAL:
		return KERN_ERR;
	default:
		return KERN_INFO;
	}
}

static const char *pdm_log_basename(const char *path)
{
	const char *name;

	if (!path) {
		return "";
	}

	name = strrchr(path, '/');
	return name ? name + 1 : path;
}

void pdm_log_set_level(int level)
{
	g_log_level = level;
}

void pdm_log_get_stats(u64 *total_count, u64 *dropped_count)
{
	if (total_count) {
		*total_count = (u64)atomic64_read(&g_log_total);
	}
	if (dropped_count) {
		*dropped_count = (u64)atomic64_read(&g_log_dropped);
	}
}

void pdm_log_emit(int level, const char *file, int line, const char *format, ...)
{
	va_list args;
	char message[PDM_LOG_MESSAGE_SIZE];
	const char *filename = pdm_log_basename(file);

	if (level < g_log_level) {
		atomic64_inc(&g_log_dropped);
		return;
	}

	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	atomic64_inc(&g_log_total);
	printk("%s[PDM:%s] %s:%d - %s\n", pdm_log_level_prefix(level),
	       pdm_log_level_name(level), filename, line, message);
}
