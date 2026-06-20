/************************************************************************
 * Kernel OSAL log API
 ************************************************************************/

#ifndef OSAL_LOG_H
#define OSAL_LOG_H

#include "osal_types.h"

#define OS_LOG_LEVEL_DEBUG 0x0
#define OS_LOG_LEVEL_INFO 0x1
#define OS_LOG_LEVEL_WARN 0x2
#define OS_LOG_LEVEL_ERROR 0x3
#define OS_LOG_LEVEL_FATAL 0x4

#define OSAL_LOG_DEFAULT_LEVEL OS_LOG_LEVEL_INFO
#define OSAL_LOG_MESSAGE_SIZE 0x200U
#define OSAL_LOG_MAX_MODULES 0x20U
#define OSAL_LOG_MAX_KV_PAIRS 0x8U

#ifndef OSAL_LOG_COMPILE_LEVEL
#if !defined(CONFIG_LOGGING_ENABLE)
#define OSAL_LOG_COMPILE_LEVEL (OS_LOG_LEVEL_FATAL + 1)
#elif defined(CONFIG_LOG_LEVEL_FATAL)
#define OSAL_LOG_COMPILE_LEVEL OS_LOG_LEVEL_FATAL
#elif defined(CONFIG_LOG_LEVEL_ERROR)
#define OSAL_LOG_COMPILE_LEVEL OS_LOG_LEVEL_ERROR
#elif defined(CONFIG_LOG_LEVEL_WARN)
#define OSAL_LOG_COMPILE_LEVEL OS_LOG_LEVEL_WARN
#elif defined(CONFIG_LOG_LEVEL_INFO)
#define OSAL_LOG_COMPILE_LEVEL OS_LOG_LEVEL_INFO
#else
#define OSAL_LOG_COMPILE_LEVEL OS_LOG_LEVEL_DEBUG
#endif
#endif

typedef enum {
	LOG_MODULE_OSAL = 0x0,
	LOG_MODULE_LPF_HW,
	LOG_MODULE_PCONFIG,
	LOG_MODULE_PDI,
	LOG_MODULE_ACONFIG,
	LOG_MODULE_APP,
	LOG_MODULE_TEST,
	LOG_MODULE_MAX
} log_module_t;

typedef struct {
	const char *key;
	const char *value;
} log_kv_pair_t;

#define LOG_DEBUG(module, ...)                                            \
	do {                                                                  \
		if (OS_LOG_LEVEL_DEBUG >= OSAL_LOG_COMPILE_LEVEL)                 \
			osal_log_emit(OS_LOG_LEVEL_DEBUG, module, __FILE__, __func__, \
				      __LINE__, __VA_ARGS__);                         \
	} while (0)

#define LOG_INFO(module, ...)                                             \
	do {                                                                  \
		if (OS_LOG_LEVEL_INFO >= OSAL_LOG_COMPILE_LEVEL)                  \
			osal_log_emit(OS_LOG_LEVEL_INFO, module, __FILE__, __func__,  \
				      __LINE__, __VA_ARGS__);                         \
	} while (0)

#define LOG_WARN(module, ...)                                             \
	do {                                                                  \
		if (OS_LOG_LEVEL_WARN >= OSAL_LOG_COMPILE_LEVEL)                  \
			osal_log_emit(OS_LOG_LEVEL_WARN, module, __FILE__, __func__,  \
				      __LINE__, __VA_ARGS__);                         \
	} while (0)

#define LOG_ERROR(module, ...)                                            \
	do {                                                                  \
		if (OS_LOG_LEVEL_ERROR >= OSAL_LOG_COMPILE_LEVEL)                 \
			osal_log_emit(OS_LOG_LEVEL_ERROR, module, __FILE__, __func__, \
				      __LINE__, __VA_ARGS__);                         \
	} while (0)

#define LOG_FATAL(module, ...)                                            \
	do {                                                                  \
		if (OS_LOG_LEVEL_FATAL >= OSAL_LOG_COMPILE_LEVEL)                 \
			osal_log_emit(OS_LOG_LEVEL_FATAL, module, __FILE__, __func__, \
				      __LINE__, __VA_ARGS__);                         \
	} while (0)

#define LOG_DEBUG_ONCE(module, ...)                                       \
	do {                                                                  \
		static bool __logged;                                             \
		if (!__logged && OS_LOG_LEVEL_DEBUG >= OSAL_LOG_COMPILE_LEVEL) {  \
			__logged = true;                                          \
			osal_log_emit(OS_LOG_LEVEL_DEBUG, module, __FILE__,        \
				      __func__, __LINE__, __VA_ARGS__);            \
		}                                                                 \
	} while (0)

#define LOG_WARN_ONCE(module, ...)                                        \
	do {                                                                  \
		static bool __logged;                                             \
		if (!__logged && OS_LOG_LEVEL_WARN >= OSAL_LOG_COMPILE_LEVEL) {   \
			__logged = true;                                          \
			osal_log_emit(OS_LOG_LEVEL_WARN, module, __FILE__,         \
				      __func__, __LINE__, __VA_ARGS__);            \
		}                                                                 \
	} while (0)

#define LOG_ERROR_ONCE(module, ...)                                       \
	do {                                                                  \
		static bool __logged;                                             \
		if (!__logged && OS_LOG_LEVEL_ERROR >= OSAL_LOG_COMPILE_LEVEL) {  \
			__logged = true;                                          \
			osal_log_emit(OS_LOG_LEVEL_ERROR, module, __FILE__,        \
				      __func__, __LINE__, __VA_ARGS__);            \
		}                                                                 \
	} while (0)

int32_t osal_log_init(const char *log_file_path, int32_t level);
void osal_log_shutdown(void);
void osal_log_set_level(int32_t level);
void osal_log_set_max_file_size(uint32_t size_bytes);
void osal_log_set_max_files(uint32_t max_files);
void osal_log_set_module_level(log_module_t module, int32_t level);
int32_t osal_log_get_module_level(log_module_t module);
int32_t osal_log_set_filter(const char *pattern);
void osal_log_set_sampling(uint32_t rate);
int32_t osal_log_set_remote(const char *host, uint16_t port);
void osal_log_disable_remote(void);
void osal_log(int32_t level, const char *module, const char *format, ...);
void osal_log_structured(int32_t level, log_module_t module,
			 const char *message, const log_kv_pair_t *kv_pairs,
			 uint32_t kv_count);
void osal_printf(const char *format, ...);
void osal_log_get_stats(uint64_t *total_count, uint64_t *dropped_count);
void osal_log_emit(int32_t level, const char *module, const char *file,
		   const char *func, int32_t line, const char *format, ...)
	__printf(6, 7);

#endif /* OSAL_LOG_H */
