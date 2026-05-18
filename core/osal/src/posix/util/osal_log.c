/************************************************************************
 * OSAL POSIX实现 - 日志系统
 *
 * 提供分级日志功能
 ************************************************************************/

#include "osal.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>

/*
 * 日志级别
 */
typedef enum
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

/*
 * 日志配置
 */
static log_level_t g_log_level = LOG_LEVEL_INFO;
static FILE *g_log_file = NULL;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_log_file_path[OSAL_LOG_PATH_SIZE] = {0};
static uint32_t g_max_log_size = OSAL_LOG_FILE_MAX_SIZE_MB * 1024 * 1024;  /* 10MB */
static uint32_t g_max_log_files = OSAL_LOG_FILE_BACKUP_COUNT;

/*
 * 日志级别名称
 */
static const char *log_level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

/*
 * 日志级别颜色（终端）
 */
static const char *log_level_colors[] = {
    "\033[36m",  // DEBUG - 青色
    "\033[32m",  // INFO  - 绿色
    "\033[33m",  // WARN  - 黄色
    "\033[31m",  // ERROR - 红色
    "\033[35m"   // FATAL - 紫色
};

static const char *color_reset = "\033[0m";

/**
 * @brief 初始化日志系统
 *
 * @param[in] log_file_path 日志文件路径，NULL表示只输出到终端
 * @param[in] level 日志级别
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_LogInit(const char *log_file_path, int32_t level)
{
    if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_FATAL)
    {
        g_log_level = level;
    }

    if (NULL != log_file_path)
    {
        strncpy(g_log_file_path, log_file_path, sizeof(g_log_file_path) - 1);
        g_log_file_path[sizeof(g_log_file_path) - 1] = '\0';
        g_log_file = fopen(log_file_path, "a");
        if (NULL == g_log_file)
        {
            fprintf(stderr, "无法打开日志文件: %s\n", log_file_path);
            return OSAL_ERR_GENERIC;
        }
    }

    return OSAL_SUCCESS;
}

/**
 * @brief 关闭日志系统
 */
void OSAL_LogShutdown(void)
{
    if (NULL != g_log_file)
    {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

/**
 * @brief 设置日志级别
 */
void OSAL_LogSetLevel(int32_t level)
{
    if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_FATAL)
    {
        g_log_level = level;
    }
}

/**
 * @brief 设置日志文件最大大小
 */
void OSAL_LogSetMaxFileSize(uint32_t size_bytes)
{
    if (size_bytes > 0)
        g_max_log_size = size_bytes;
}

/**
 * @brief 设置最大日志文件数
 */
void OSAL_LogSetMaxFiles(uint32_t max_files)
{
    if (max_files > 0)
        g_max_log_files = max_files;
}

/**
 * @brief 获取当前时间字符串
 */
static void get_timestamp(char *buffer, osal_size_t size)
{
    struct timeval tv;
    struct tm tm_info;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm_info);  /* 线程安全版本 */

    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm_info.tm_year + 1900,
             tm_info.tm_mon + 1,
             tm_info.tm_mday,
             tm_info.tm_hour,
             tm_info.tm_min,
             tm_info.tm_sec,
             (int)(tv.tv_usec / OSAL_USEC_PER_MSEC));
}

/**
 * @brief 轮转日志文件
 */
static void rotate_log_file(void)
{
    if (g_log_file_path[0] == '\0')
        return;

    /* 关闭当前日志文件 */
    if (NULL != g_log_file)
    {
        fclose(g_log_file);
        g_log_file = NULL;
    }

    /* 删除最旧的日志文件 */
    char old_file[OSAL_LOG_FILENAME_SIZE];
    snprintf(old_file, sizeof(old_file), "%s.%u", g_log_file_path, g_max_log_files);
    if (remove(old_file) != 0 && errno != ENOENT)
    {
        /* 文件不存在是正常情况，其他错误记录到 stderr */
        fprintf(stderr, "[LOG] 警告：无法删除旧日志文件 %s: %s\n", old_file, strerror(errno));
    }

    /* 重命名日志文件 */
    for (uint32_t i = g_max_log_files - 1; i > 0; i--)
    {
        char from[OSAL_LOG_FILENAME_SIZE], to[OSAL_LOG_FILENAME_SIZE];
        snprintf(from, sizeof(from), "%s.%u", g_log_file_path, i - 1);
        snprintf(to, sizeof(to), "%s.%u", g_log_file_path, i);
        if (rename(from, to) != 0 && errno != ENOENT)
        {
            fprintf(stderr, "[LOG] 警告：无法重命名日志文件 %s -> %s: %s\n",
                    from, to, strerror(errno));
        }
    }

    /* 重命名当前日志文件 */
    char current_backup[OSAL_LOG_FILENAME_SIZE];
    snprintf(current_backup, sizeof(current_backup), "%s.1", g_log_file_path);
    if (rename(g_log_file_path, current_backup) != 0)
    {
        fprintf(stderr, "[LOG] 警告：无法重命名当前日志文件 %s -> %s: %s\n",
                g_log_file_path, current_backup, strerror(errno));
    }

    /* 打开新的日志文件 */
    g_log_file = fopen(g_log_file_path, "a");
    if (NULL != g_log_file)
    {
        fprintf(g_log_file, "[LOG ROTATION] Log file rotated\n");
        fflush(g_log_file);
    }
    else
    {
        fprintf(stderr, "[LOG] 错误：无法打开新日志文件 %s: %s\n",
                g_log_file_path, strerror(errno));
    }
}

/**
 * @brief 检查日志文件大小并轮转
 */
static void check_and_rotate_log(void)
{
    if (NULL == g_log_file || g_log_file_path[0] == '\0')
        return;

    /* 获取文件大小 */
    fseek(g_log_file, 0, SEEK_END);
    int64_t file_size = (int64_t)ftell(g_log_file);

    if (file_size > (int64_t)g_max_log_size)
    {
        rotate_log_file();
    }
}

/**
 * @brief 提取文件名（去掉路径）
 */
static const char *extract_filename(const char *path)
{
    const char *filename = strrchr(path, '/');
    return filename ? filename + 1 : path;
}

/**
 * @brief 内部日志函数（带位置信息）
 */
static void log_internal_ex(log_level_t level, const char *module,
                            const char *file, const char *func, int32_t line,
                            const char *format, va_list args)
{
    char timestamp[OSAL_LOG_TIMESTAMP_SIZE];
    char message[OSAL_LOG_MESSAGE_SIZE];
    const char *filename = extract_filename(file);

    /* 检查日志级别 */
    if (level < g_log_level)
        return;

    /* 获取时间戳 */
    get_timestamp(timestamp, sizeof(timestamp));

    /* 格式化消息 */
    vsnprintf(message, sizeof(message), format, args);

    /* 加锁 */
    pthread_mutex_lock(&g_log_mutex);

    /* 检查并轮转日志文件 */
    check_and_rotate_log();

    /* 输出到终端（带颜色） */
    printf("%s[%s] [%s] [%s] [%s:%s:%d]%s %s\n",
           log_level_colors[level],
           timestamp,
           log_level_names[level],
           module,
           filename,
           func,
           line,
           color_reset,
           message);

    /* 输出到文件（无颜色） */
    if (NULL != g_log_file)
    {
        fprintf(g_log_file, "[%s] [%s] [%s] [%s:%s:%d] %s\n",
               timestamp,
               log_level_names[level],
               module,
               filename,
               func,
               line,
               message);
        fflush(g_log_file);
    }

    /* 解锁 */
    pthread_mutex_unlock(&g_log_mutex);
}

/**
 * @brief 内部日志函数（不带位置信息，兼容旧接口）
 */
static void log_internal(log_level_t level, const char *module,
                         const char *format, va_list args)
{
    char timestamp[OSAL_LOG_TIMESTAMP_SIZE];
    char message[OSAL_LOG_MESSAGE_SIZE];

    /* 检查日志级别 */
    if (level < g_log_level)
        return;

    /* 获取时间戳 */
    get_timestamp(timestamp, sizeof(timestamp));

    /* 格式化消息 */
    vsnprintf(message, sizeof(message), format, args);

    /* 加锁 */
    pthread_mutex_lock(&g_log_mutex);

    /* 检查并轮转日志文件 */
    check_and_rotate_log();

    /* 输出到终端（带颜色） */
    if (NULL != module)
    {
        printf("%s[%s] [%s] [%s]%s %s\n",
               log_level_colors[level],
               timestamp,
               log_level_names[level],
               module,
               color_reset,
               message);
    }
    else
    {
        printf("%s[%s] [%s]%s %s\n",
               log_level_colors[level],
               timestamp,
               log_level_names[level],
               color_reset,
               message);
    }

    /* 输出到文件（无颜色） */
    if (NULL != g_log_file)
    {
        if (NULL != module)
        {
            fprintf(g_log_file, "[%s] [%s] [%s] %s\n",
                   timestamp,
                   log_level_names[level],
                   module,
                   message);
        }
        else
        {
            fprintf(g_log_file, "[%s] [%s] %s\n",
                   timestamp,
                   log_level_names[level],
                   message);
        }
        fflush(g_log_file);
    }

    /* 解锁 */
    pthread_mutex_unlock(&g_log_mutex);
}

/**
 * @brief 通用日志函数
 */
void OSAL_Log(int32_t level, const char *module, const char *format, ...)
{
    va_list args;

    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
        return;

    va_start(args, format);
    log_internal(level, module, format, args);
    va_end(args);
}

/**
 * @brief DEBUG级别日志（带位置信息）
 */
void OSAL_LogDebug(const char *module, const char *file, const char *func, int32_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_internal_ex(LOG_LEVEL_DEBUG, module, file, func, line, format, args);
    va_end(args);
}

/**
 * @brief INFO级别日志（带位置信息）
 */
void OSAL_LogInfo(const char *module, const char *file, const char *func, int32_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_internal_ex(LOG_LEVEL_INFO, module, file, func, line, format, args);
    va_end(args);
}

/**
 * @brief WARN级别日志（带位置信息）
 */
void OSAL_LogWarn(const char *module, const char *file, const char *func, int32_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_internal_ex(LOG_LEVEL_WARN, module, file, func, line, format, args);
    va_end(args);
}

/**
 * @brief ERROR级别日志（带位置信息）
 */
void OSAL_LogError(const char *module, const char *file, const char *func, int32_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_internal_ex(LOG_LEVEL_ERROR, module, file, func, line, format, args);
    va_end(args);
}

/**
 * @brief FATAL级别日志（带位置信息）
 */
void OSAL_LogFatal(const char *module, const char *file, const char *func, int32_t line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_internal_ex(LOG_LEVEL_FATAL, module, file, func, line, format, args);
    va_end(args);
}

/**
 * @brief 简单打印（不带日志级别和模块名）
 */
void OSAL_Printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}
