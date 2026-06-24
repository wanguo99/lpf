// SPDX-License-Identifier: GPL-2.0

#include "osal/osal.h"

#include <linux/atomic.h>
#include <linux/printk.h>
#include <linux/string.h>

static atomic64_t g_log_total = ATOMIC64_INIT(0);
static atomic64_t g_log_dropped = ATOMIC64_INIT(0);
static int32_t g_log_level = OSAL_LOG_DEFAULT_LEVEL;
static int32_t g_module_levels[LOG_MODULE_MAX] = {
	[0 ... LOG_MODULE_MAX - 1] = OSAL_LOG_DEFAULT_LEVEL,
};

static const char *osal_log_level_name(int32_t level)
{
	switch (level) {
	case OS_LOG_LEVEL_DEBUG:
		return "DEBUG";
	case OS_LOG_LEVEL_INFO:
		return "INFO";
	case OS_LOG_LEVEL_WARN:
		return "WARN";
	case OS_LOG_LEVEL_ERROR:
		return "ERROR";
	case OS_LOG_LEVEL_FATAL:
		return "FATAL";
	default:
		return "UNKNOWN";
	}
}

static const char *osal_log_level_prefix(int32_t level)
{
	switch (level) {
	case OS_LOG_LEVEL_DEBUG:
		return KERN_DEBUG;
	case OS_LOG_LEVEL_INFO:
		return KERN_INFO;
	case OS_LOG_LEVEL_WARN:
		return KERN_WARNING;
	case OS_LOG_LEVEL_ERROR:
	case OS_LOG_LEVEL_FATAL:
		return KERN_ERR;
	default:
		return KERN_INFO;
	}
}

static const char *osal_log_basename(const char *path)
{
	const char *name;

	if (!path)
		return "";

	name = strrchr(path, '/');
	return name ? name + 1 : path;
}

int32_t osal_log_init(const char *log_file_path, int32_t level)
{
	(void)log_file_path;
	g_log_level = level;
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_log_init);

void osal_log_shutdown(void)
{
}
EXPORT_SYMBOL_GPL(osal_log_shutdown);

void osal_log_set_level(int32_t level)
{
	g_log_level = level;
}
EXPORT_SYMBOL_GPL(osal_log_set_level);

void osal_log_set_max_file_size(uint32_t size_bytes)
{
	(void)size_bytes;
}
EXPORT_SYMBOL_GPL(osal_log_set_max_file_size);

void osal_log_set_max_files(uint32_t max_files)
{
	(void)max_files;
}
EXPORT_SYMBOL_GPL(osal_log_set_max_files);

void osal_log_set_module_level(log_module_t module, int32_t level)
{
	if (module >= 0 && module < LOG_MODULE_MAX)
		g_module_levels[module] = level;
}
EXPORT_SYMBOL_GPL(osal_log_set_module_level);

int32_t osal_log_get_module_level(log_module_t module)
{
	if (module >= 0 && module < LOG_MODULE_MAX)
		return g_module_levels[module];

	return g_log_level;
}
EXPORT_SYMBOL_GPL(osal_log_get_module_level);

int32_t osal_log_set_filter(const char *pattern)
{
	(void)pattern;
	return OSAL_ERR_NOT_SUPPORTED;
}
EXPORT_SYMBOL_GPL(osal_log_set_filter);

void osal_log_set_sampling(uint32_t rate)
{
	(void)rate;
}
EXPORT_SYMBOL_GPL(osal_log_set_sampling);

int32_t osal_log_set_remote(const char *host, uint16_t port)
{
	(void)host;
	(void)port;
	return OSAL_ERR_NOT_SUPPORTED;
}
EXPORT_SYMBOL_GPL(osal_log_set_remote);

void osal_log_disable_remote(void)
{
}
EXPORT_SYMBOL_GPL(osal_log_disable_remote);

void osal_log(int32_t level, const char *module, const char *format, ...)
{
	va_list args;
	char message[OSAL_LOG_MESSAGE_SIZE];

	(void)module;

	if (level < g_log_level) {
		atomic64_inc(&g_log_dropped);
		return;
	}

	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	atomic64_inc(&g_log_total);
	printk("%s[%s] - %s\n", osal_log_level_prefix(level),
	       osal_log_level_name(level), message);
}
EXPORT_SYMBOL_GPL(osal_log);

void osal_log_structured(int32_t level, log_module_t module,
			 const char *message, const log_kv_pair_t *kv_pairs,
			 uint32_t kv_count)
{
	uint32_t i;

	osal_log(level, "STRUCT", "module=%d message=%s", module,
		 message ? message : "");
	for (i = 0; i < kv_count; i++) {
		if (kv_pairs[i].key)
			osal_log(level, "STRUCT", "%s=%s", kv_pairs[i].key,
				 kv_pairs[i].value ? kv_pairs[i].value : "");
	}
}
EXPORT_SYMBOL_GPL(osal_log_structured);

void osal_printf(const char *format, ...)
{
	va_list args;
	char message[OSAL_LOG_MESSAGE_SIZE];

	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	printk(KERN_INFO "%s", message);
}
EXPORT_SYMBOL_GPL(osal_printf);

void osal_log_get_stats(uint64_t *total_count, uint64_t *dropped_count)
{
	if (total_count)
		*total_count = (uint64_t)atomic64_read(&g_log_total);
	if (dropped_count)
		*dropped_count = (uint64_t)atomic64_read(&g_log_dropped);
}
EXPORT_SYMBOL_GPL(osal_log_get_stats);

void osal_log_emit(int32_t level, const char *module, const char *file,
		   const char *func, int32_t line, const char *format, ...)
{
	va_list args;
	char message[OSAL_LOG_MESSAGE_SIZE];
	const char *filename = osal_log_basename(file);

	(void)module;
	(void)func;

	if (level < g_log_level) {
		atomic64_inc(&g_log_dropped);
		return;
	}

	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	atomic64_inc(&g_log_total);
	printk("%s[%s] %s:%d - %s\n", osal_log_level_prefix(level),
	       osal_log_level_name(level), filename, line, message);
}
EXPORT_SYMBOL_GPL(osal_log_emit);
