/************************************************************************
 * OSAL日志API头文件
 ************************************************************************/

#ifndef OSAPI_LOG_H
#define OSAPI_LOG_H

#include "osal_types.h"

/*
 * 日志级别
 */
#define OS_LOG_LEVEL_DEBUG  0
#define OS_LOG_LEVEL_INFO   1
#define OS_LOG_LEVEL_WARN   2
#define OS_LOG_LEVEL_ERROR  3
#define OS_LOG_LEVEL_FATAL  4

/* 日志配置 */
#define OSAL_LOG_DEFAULT_LEVEL       OS_LOG_LEVEL_INFO
#define OSAL_LOG_FILE_PATH           "/var/log/ems.log"
#define OSAL_LOG_FILE_MAX_SIZE_MB    10
#define OSAL_LOG_FILE_BACKUP_COUNT   5

/* 日志缓冲区大小 */
#define OSAL_LOG_PATH_SIZE           256U   /* 日志路径缓冲区大小 */
#define OSAL_LOG_FILENAME_SIZE       512U   /* 日志文件名缓冲区大小 */
#define OSAL_LOG_TIMESTAMP_SIZE      64U    /* 时间戳缓冲区大小 */
#define OSAL_LOG_MESSAGE_SIZE        1024U  /* 日志消息缓冲区大小 */

/**
 * @brief 初始化日志系统
 *
 * @param[in] log_file_path 日志文件路径，NULL表示只输出到终端
 * @param[in] level 日志级别
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t OSAL_LogInit(const char *log_file_path, int32_t level);

/**
 * @brief 关闭日志系统
 */
void OSAL_LogShutdown(void);

/**
 * @brief 设置日志级别
 *
 * @param[in] level 日志级别
 */
void OSAL_LogSetLevel(int32_t level);

/**
 * @brief 设置日志文件最大大小
 *
 * @param[in] size_bytes 最大文件大小（字节）
 */
void OSAL_LogSetMaxFileSize(uint32_t size_bytes);

/**
 * @brief 设置最大日志文件数
 *
 * @param[in] max_files 最大备份文件数
 */
void OSAL_LogSetMaxFiles(uint32_t max_files);

/**
 * @brief 通用日志函数
 *
 * @param[in] level 日志级别
 * @param[in] module 模块名称
 * @param[in] format 格式化字符串
 * @param[in] ... 可变参数
 */
void OSAL_Log(int32_t level, const char *module, const char *format, ...);

/**
 * @brief 简单打印（不带日志级别和模块名）
 */
void OSAL_Printf(const char *format, ...);

/*
 * 日志宏定义（推荐使用）
 *
 * 注意：请使用这些宏而不是直接调用 OSAL_Log* 函数
 * 宏会自动添加文件名、函数名、行号信息
 */
#define LOG_DEBUG(module, ...) OSAL_LogDebug(module, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_INFO(module, ...)  OSAL_LogInfo(module, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_WARN(module, ...)  OSAL_LogWarn(module, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(module, ...) OSAL_LogError(module, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(module, ...) OSAL_LogFatal(module, __FILE__, __func__, __LINE__, __VA_ARGS__)

/*
 * 内部实现函数（仅供宏使用，请勿直接调用）
 */
void OSAL_LogDebug(const char *module, const char *file, const char *func, int32_t line, const char *format, ...);
void OSAL_LogInfo(const char *module, const char *file, const char *func, int32_t line, const char *format, ...);
void OSAL_LogWarn(const char *module, const char *file, const char *func, int32_t line, const char *format, ...);
void OSAL_LogError(const char *module, const char *file, const char *func, int32_t line, const char *format, ...);
void OSAL_LogFatal(const char *module, const char *file, const char *func, int32_t line, const char *format, ...);

#endif /* OSAPI_LOG_H */
