/* SPDX-License-Identifier: GPL-2.0 */

#ifndef PDM_LOG_H
#define PDM_LOG_H

#include <linux/compiler_attributes.h>
#include <linux/types.h>

#define PDM_LOG_LEVEL_DEBUG 0
#define PDM_LOG_LEVEL_INFO 1
#define PDM_LOG_LEVEL_WARN 2
#define PDM_LOG_LEVEL_ERROR 3
#define PDM_LOG_LEVEL_FATAL 4

#define PDM_LOG_DEFAULT_LEVEL PDM_LOG_LEVEL_INFO
#define PDM_LOG_MESSAGE_SIZE 512U

#ifndef PDM_LOG_COMPILE_LEVEL
#if !defined(CONFIG_PDM_LOGGING_ENABLE)
#define PDM_LOG_COMPILE_LEVEL (PDM_LOG_LEVEL_FATAL + 1)
#elif defined(CONFIG_PDM_LOG_LEVEL_FATAL)
#define PDM_LOG_COMPILE_LEVEL PDM_LOG_LEVEL_FATAL
#elif defined(CONFIG_PDM_LOG_LEVEL_ERROR)
#define PDM_LOG_COMPILE_LEVEL PDM_LOG_LEVEL_ERROR
#elif defined(CONFIG_PDM_LOG_LEVEL_WARN)
#define PDM_LOG_COMPILE_LEVEL PDM_LOG_LEVEL_WARN
#elif defined(CONFIG_PDM_LOG_LEVEL_INFO)
#define PDM_LOG_COMPILE_LEVEL PDM_LOG_LEVEL_INFO
#else
#define PDM_LOG_COMPILE_LEVEL PDM_LOG_LEVEL_DEBUG
#endif
#endif

#define LOG_DEBUG(...)                                                     \
	do {                                                               \
		if (PDM_LOG_LEVEL_DEBUG >= PDM_LOG_COMPILE_LEVEL)          \
			pdm_log_emit(PDM_LOG_LEVEL_DEBUG, __FILE__,        \
				     __LINE__, __VA_ARGS__);               \
	} while (0)

#define LOG_INFO(...)                                                      \
	do {                                                               \
		if (PDM_LOG_LEVEL_INFO >= PDM_LOG_COMPILE_LEVEL)           \
			pdm_log_emit(PDM_LOG_LEVEL_INFO, __FILE__,         \
				     __LINE__, __VA_ARGS__);               \
	} while (0)

#define LOG_WARN(...)                                                      \
	do {                                                               \
		if (PDM_LOG_LEVEL_WARN >= PDM_LOG_COMPILE_LEVEL)           \
			pdm_log_emit(PDM_LOG_LEVEL_WARN, __FILE__,         \
				     __LINE__, __VA_ARGS__);               \
	} while (0)

#define LOG_ERROR(...)                                                     \
	do {                                                               \
		if (PDM_LOG_LEVEL_ERROR >= PDM_LOG_COMPILE_LEVEL)          \
			pdm_log_emit(PDM_LOG_LEVEL_ERROR, __FILE__,        \
				     __LINE__, __VA_ARGS__);               \
	} while (0)

#define LOG_FATAL(...)                                                     \
	do {                                                               \
		if (PDM_LOG_LEVEL_FATAL >= PDM_LOG_COMPILE_LEVEL)          \
			pdm_log_emit(PDM_LOG_LEVEL_FATAL, __FILE__,        \
				     __LINE__, __VA_ARGS__);               \
	} while (0)

#define LOG_DEBUG_ONCE(...)                                                \
	do {                                                               \
		static bool __logged;                                      \
		if (!__logged &&                                          \
		    PDM_LOG_LEVEL_DEBUG >= PDM_LOG_COMPILE_LEVEL) {       \
			__logged = true;                                  \
			pdm_log_emit(PDM_LOG_LEVEL_DEBUG, __FILE__,       \
				     __LINE__, __VA_ARGS__);              \
		}                                                          \
	} while (0)

#define LOG_WARN_ONCE(...)                                                 \
	do {                                                               \
		static bool __logged;                                      \
		if (!__logged &&                                          \
		    PDM_LOG_LEVEL_WARN >= PDM_LOG_COMPILE_LEVEL) {        \
			__logged = true;                                  \
			pdm_log_emit(PDM_LOG_LEVEL_WARN, __FILE__,        \
				     __LINE__, __VA_ARGS__);              \
		}                                                          \
	} while (0)

#define LOG_ERROR_ONCE(...)                                                \
	do {                                                               \
		static bool __logged;                                      \
		if (!__logged &&                                          \
		    PDM_LOG_LEVEL_ERROR >= PDM_LOG_COMPILE_LEVEL) {       \
			__logged = true;                                  \
			pdm_log_emit(PDM_LOG_LEVEL_ERROR, __FILE__,       \
				     __LINE__, __VA_ARGS__);              \
		}                                                          \
	} while (0)

void pdm_log_set_level(int level);
void pdm_log_get_stats(u64 *total_count, u64 *dropped_count);
void pdm_log_emit(int level, const char *file, int line, const char *format, ...)
	__printf(4, 5);

#endif /* PDM_LOG_H */
