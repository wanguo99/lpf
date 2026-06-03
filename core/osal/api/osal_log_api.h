/************************************************************************
 * OSAL Public API - Logging System
 *
 * This file provides the public API for structured logging with
 * filtering, sampling, and remote logging capabilities.
 ************************************************************************/

#ifndef OSAL_LOG_API_H
#define OSAL_LOG_API_H

#include "osal_types_api.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
 * Log System Management
 ************************************************************************/

/**
 * @brief Initialize the logging system
 *
 * Sets up the logging system with specified file path and log level.
 *
 * @param log_file_path Path to log file (NULL for console-only logging)
 * @param level Initial log level (LOG_LEVEL_DEBUG..LOG_LEVEL_FATAL)
 * @return OSAL_SUCCESS on success, error code on failure
 *
 * @note Must be called before any logging functions
 * @note Thread-safe
 */
int32_t OSAL_LogInit(const char *log_file_path, int32_t level);

/**
 * @brief Shutdown the logging system
 *
 * Closes log files and releases resources. Should be called at
 * program termination.
 *
 * @note Thread-safe
 */
void OSAL_LogShutdown(void);

/************************************************************************
 * Log Level Configuration
 ************************************************************************/

/**
 * @brief Set global log level
 *
 * Sets the minimum log level for all modules. Messages below this
 * level will be filtered out.
 *
 * @param level Log level (LOG_LEVEL_DEBUG..LOG_LEVEL_FATAL)
 *
 * @note Thread-safe
 */
void OSAL_LogSetLevel(int32_t level);

/**
 * @brief Set log level for specific module
 *
 * Overrides the global log level for a specific module.
 *
 * @param module Module identifier (LOG_MODULE_*)
 * @param level Log level for this module
 *
 * @note Thread-safe
 */
void OSAL_LogSetModuleLevel(log_module_t module, int32_t level);

/**
 * @brief Get log level for specific module
 *
 * Returns the configured log level for a module, or the global
 * level if no module-specific level is set.
 *
 * @param module Module identifier
 * @return Log level for this module
 *
 * @note Thread-safe
 */
int32_t OSAL_LogGetModuleLevel(log_module_t module);

/************************************************************************
 * File Management
 ************************************************************************/

/**
 * @brief Set maximum log file size
 *
 * When the current log file exceeds this size, it will be rotated.
 *
 * @param size_bytes Maximum size in bytes (default: 10MB)
 *
 * @note Thread-safe
 */
void OSAL_LogSetMaxFileSize(uint32_t size_bytes);

/**
 * @brief Set maximum number of backup log files
 *
 * Controls how many rotated log files to keep.
 *
 * @param max_files Number of backup files (default: 5)
 *
 * @note Thread-safe
 */
void OSAL_LogSetMaxFiles(uint32_t max_files);

/************************************************************************
 * Filtering and Sampling
 ************************************************************************/

/**
 * @brief Set log filter pattern
 *
 * Only log messages matching the regex pattern will be output.
 *
 * @param pattern POSIX regex pattern (NULL to disable filtering)
 * @return OSAL_SUCCESS on success, error code on failure
 *
 * @note Thread-safe
 */
int32_t OSAL_LogSetFilter(const char *pattern);

/**
 * @brief Set log sampling rate
 *
 * Only 1 out of every N messages will be logged. Used to reduce
 * log volume from high-frequency events.
 *
 * @param rate Sampling rate (1 = log all, 10 = log 1 in 10, etc.)
 *
 * @note Thread-safe
 */
void OSAL_LogSetSampling(uint32_t rate);

/************************************************************************
 * Remote Logging
 ************************************************************************/

/**
 * @brief Enable remote logging via UDP
 *
 * Sends log messages to a remote syslog server.
 *
 * @param host IP address or hostname
 * @param port UDP port (typically 514 for syslog)
 * @return OSAL_SUCCESS on success, error code on failure
 *
 * @note Thread-safe
 */
int32_t OSAL_LogSetRemote(const char *host, uint16_t port);

/**
 * @brief Disable remote logging
 *
 * Stops sending logs to remote server and closes the socket.
 *
 * @note Thread-safe
 */
void OSAL_LogDisableRemote(void);

/************************************************************************
 * Statistics
 ************************************************************************/

/**
 * @brief Get logging statistics
 *
 * Returns total number of log messages and dropped messages.
 *
 * @param[out] total_count Total messages logged
 * @param[out] dropped_count Messages dropped (sampling/filtering)
 *
 * @note Thread-safe
 */
void OSAL_LogGetStats(uint64_t *total_count, uint64_t *dropped_count);

/************************************************************************
 * Logging Functions
 ************************************************************************/

/**
 * @brief Log a message
 *
 * Generic logging function with printf-style formatting.
 *
 * @param level Log level
 * @param module Module name (string)
 * @param format Printf-style format string
 * @param ... Variable arguments
 *
 * @note Thread-safe
 */
void OSAL_Log(int32_t level, const char *module, const char *format, ...);

/**
 * @brief Log structured message with key-value pairs
 *
 * Logs a message with associated key-value metadata for structured
 * logging systems.
 *
 * @param level Log level
 * @param module Module identifier
 * @param message Main log message
 * @param key_values Key-value pairs (format: "key1=value1 key2=value2")
 *
 * @note Thread-safe
 */
void OSAL_LogStructured(int32_t level, log_module_t module, const char *message,
                        const char *key_values);

/**
 * @brief Print to console and log file
 *
 * Similar to printf, but outputs to both console and log file if
 * logging is initialized.
 *
 * @param format Printf-style format string
 * @param ... Variable arguments
 * @return Number of characters written, or negative on error
 *
 * @note Thread-safe
 */
int32_t OSAL_Printf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_LOG_API_H */
