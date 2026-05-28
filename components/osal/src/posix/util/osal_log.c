/************************************************************************
 * OSAL POSIX实现 - 日志系统
 *
 * 功能特性：
 * - 分级日志（DEBUG/INFO/WARN/ERROR/FATAL）
 * - 模块级日志控制
 * - 结构化日志支持
 * - 日志过滤和采样
 * - 远程日志传输（UDP）
 * - 自动日志轮转
 ************************************************************************/

#include "osal/osal.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex.h>

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
static log_level_t g_module_levels[LOG_MODULE_MAX];  /* 模块级日志级别 */
static bool g_module_level_set[LOG_MODULE_MAX] = {false};  /* 是否设置了模块级别 */
static FILE *g_log_file = NULL;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_log_file_path[OSAL_LOG_PATH_SIZE] = {0};
static uint32_t g_max_log_size = OSAL_LOG_FILE_MAX_SIZE_MB * 1024 * 1024;  /* 10MB */
static uint32_t g_max_log_files = OSAL_LOG_FILE_BACKUP_COUNT;

/* 日志过滤和采样 */
static regex_t g_log_filter_regex;
static bool g_log_filter_enabled = false;
static uint32_t g_log_sampling_rate = 1;  /* 1表示全部记录 */
static uint64_t g_log_counter = 0;

/* 远程日志 */
static int g_remote_log_socket = -1;
static struct sockaddr_in g_remote_log_addr;
static bool g_remote_log_enabled = false;

/* 统计信息 */
static uint64_t g_total_log_count = 0;
static uint64_t g_dropped_log_count = 0;

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
 * 模块名称
 */
static const char *log_module_names[] = {
    "OSAL",
    "HAL",
    "PCL",
    "PDL",
    "ACL",
    "APP",
    "TEST"
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
    uint32_t i;

    if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_FATAL)
    {
        g_log_level = level;
    }

    /* 初始化模块级别为未设置 */
    for (i = 0; i < LOG_MODULE_MAX; i++)
    {
        g_module_level_set[i] = false;
        g_module_levels[i] = LOG_LEVEL_INFO;
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

    /* 关闭远程日志 */
    if (g_remote_log_enabled && g_remote_log_socket >= 0)
    {
        close(g_remote_log_socket);
        g_remote_log_socket = -1;
        g_remote_log_enabled = false;
    }

    /* 清理过滤器 */
    if (g_log_filter_enabled)
    {
        regfree(&g_log_filter_regex);
        g_log_filter_enabled = false;
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
 * @brief 设置模块日志级别
 */
void OSAL_LogSetModuleLevel(log_module_t module, int32_t level)
{
    if (module < LOG_MODULE_MAX && level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_FATAL)
    {
        g_module_levels[module] = level;
        g_module_level_set[module] = true;
    }
}

/**
 * @brief 获取模块日志级别
 */
int32_t OSAL_LogGetModuleLevel(log_module_t module)
{
    if (module < LOG_MODULE_MAX && g_module_level_set[module])
    {
        return g_module_levels[module];
    }
    return g_log_level;  /* 返回全局级别 */
}

/**
 * @brief 设置日志过滤器
 */
int32_t OSAL_LogSetFilter(const char *pattern)
{
    int ret;

    /* 清除旧的过滤器 */
    if (g_log_filter_enabled)
    {
        regfree(&g_log_filter_regex);
        g_log_filter_enabled = false;
    }

    if (NULL == pattern)
    {
        return OSAL_SUCCESS;  /* 清除过滤器 */
    }

    /* 编译正则表达式 */
    ret = regcomp(&g_log_filter_regex, pattern, REG_EXTENDED | REG_NOSUB);
    if (ret != 0)
    {
        return OSAL_ERR_GENERIC;
    }

    g_log_filter_enabled = true;
    return OSAL_SUCCESS;
}

/**
 * @brief 设置日志采样率
 */
void OSAL_LogSetSampling(uint32_t rate)
{
    if (rate > 0)
    {
        g_log_sampling_rate = rate;
    }
}

/**
 * @brief 启用远程日志
 */
int32_t OSAL_LogSetRemote(const char *host, uint16_t port)
{
    if (NULL == host || port == 0)
    {
        return OSAL_ERR_INVALID_SIZE;
    }

    /* 创建UDP socket */
    g_remote_log_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_remote_log_socket < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 配置远程地址 */
    memset(&g_remote_log_addr, 0, sizeof(g_remote_log_addr));
    g_remote_log_addr.sin_family = AF_INET;
    g_remote_log_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &g_remote_log_addr.sin_addr) <= 0)
    {
        close(g_remote_log_socket);
        g_remote_log_socket = -1;
        return OSAL_ERR_GENERIC;
    }

    g_remote_log_enabled = true;
    return OSAL_SUCCESS;
}

/**
 * @brief 禁用远程日志
 */
void OSAL_LogDisableRemote(void)
{
    if (g_remote_log_enabled && g_remote_log_socket >= 0)
    {
        close(g_remote_log_socket);
        g_remote_log_socket = -1;
    }
    g_remote_log_enabled = false;
}

/**
 * @brief 获取日志统计信息
 */
void OSAL_LogGetStats(uint64_t *total_count, uint64_t *dropped_count)
{
    if (total_count)
    {
        *total_count = g_total_log_count;
    }
    if (dropped_count)
    {
        *dropped_count = g_dropped_log_count;
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
    char old_file[OSAL_LOG_FILENAME_SIZE];
    char from[OSAL_LOG_FILENAME_SIZE], to[OSAL_LOG_FILENAME_SIZE];
    char current_backup[OSAL_LOG_FILENAME_SIZE];
    uint32_t i;

    if (g_log_file_path[0] == '\0')
        return;

    /* 关闭当前日志文件 */
    if (NULL != g_log_file)
    {
        fclose(g_log_file);
        g_log_file = NULL;
    }

    /* 删除最旧的日志文件 */
    snprintf(old_file, sizeof(old_file), "%s.%u", g_log_file_path, g_max_log_files);
    if (remove(old_file) != 0 && errno != ENOENT)
    {
        /* 文件不存在是正常情况，其他错误记录到 stderr */
        fprintf(stderr, "[LOG] 警告：无法删除旧日志文件 %s: %s\n", old_file, strerror(errno));
    }

    /* 重命名日志文件 */
    for (i = g_max_log_files - 1; i > 0; i--)
    {
        snprintf(from, sizeof(from), "%s.%u", g_log_file_path, i - 1);
        snprintf(to, sizeof(to), "%s.%u", g_log_file_path, i);
        if (rename(from, to) != 0 && errno != ENOENT)
        {
            fprintf(stderr, "[LOG] 警告：无法重命名日志文件 %s -> %s: %s\n",
                    from, to, strerror(errno));
        }
    }

    /* 重命名当前日志文件 */
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
    int64_t file_size;

    if (NULL == g_log_file || g_log_file_path[0] == '\0')
        return;

    /* 获取文件大小 */
    fseek(g_log_file, 0, SEEK_END);
    file_size = (int64_t)ftell(g_log_file);

    if (file_size > (int64_t)g_max_log_size)
    {
        rotate_log_file();
    }
}

/**
 * @brief 检查日志是否应该被记录（采样和过滤）
 */
static bool should_log_message(const char *message)
{
    /* 采样检查 */
    g_log_counter++;
    if (g_log_sampling_rate > 1 && (g_log_counter % g_log_sampling_rate) != 0)
    {
        g_dropped_log_count++;
        return false;
    }

    /* 过滤器检查 */
    if (g_log_filter_enabled && message != NULL)
    {
        if (regexec(&g_log_filter_regex, message, 0, NULL, 0) != 0)
        {
            g_dropped_log_count++;
            return false;
        }
    }

    g_total_log_count++;
    return true;
}

/**
 * @brief 发送日志到远程服务器
 */
static void send_remote_log(const char *log_message)
{
    if (!g_remote_log_enabled || g_remote_log_socket < 0 || log_message == NULL)
    {
        return;
    }

    sendto(g_remote_log_socket, log_message, strlen(log_message), 0,
           (struct sockaddr *)&g_remote_log_addr, sizeof(g_remote_log_addr));
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
    char full_log[OSAL_LOG_MESSAGE_SIZE + 256];
    const char *filename = extract_filename(file);

    /* 检查日志级别 */
    if (level < g_log_level)
        return;

    /* 获取时间戳 */
    get_timestamp(timestamp, sizeof(timestamp));

    /* 格式化消息 */
    vsnprintf(message, sizeof(message), format, args);

    /* 采样和过滤检查 */
    if (!should_log_message(message))
        return;

    /* 加锁 */
    pthread_mutex_lock(&g_log_mutex);

    /* 检查并轮转日志文件 */
    check_and_rotate_log();

    /* 构造完整日志 */
    snprintf(full_log, sizeof(full_log),
             "[%s] [%s] [%s] [%s:%s:%d] %s",
             timestamp, log_level_names[level], module,
             filename, func, line, message);

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
        fprintf(g_log_file, "%s\n", full_log);
        fflush(g_log_file);
    }

    /* 发送到远程服务器 */
    send_remote_log(full_log);

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

/**
 * @brief 结构化日志函数
 */
void OSAL_LogStructured(int32_t level, log_module_t module, const char *message,
                        const log_kv_pair_t *kv_pairs, uint32_t kv_count)
{
    char timestamp[OSAL_LOG_TIMESTAMP_SIZE];
    char full_message[OSAL_LOG_MESSAGE_SIZE];
    char kv_buffer[512];
    uint32_t i;
    size_t offset = 0;

    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
        return;

    /* 检查模块级别 */
    if (module < LOG_MODULE_MAX && g_module_level_set[module])
    {
        if (level < g_module_levels[module])
            return;
    }
    else if (level < g_log_level)
    {
        return;
    }

    /* 构造键值对字符串 */
    kv_buffer[0] = '\0';
    if (kv_pairs != NULL && kv_count > 0)
    {
        for (i = 0; i < kv_count && i < OSAL_LOG_MAX_KV_PAIRS; i++)
        {
            if (kv_pairs[i].key != NULL && kv_pairs[i].value != NULL)
            {
                int written = snprintf(kv_buffer + offset, sizeof(kv_buffer) - offset,
                                      " %s=%s", kv_pairs[i].key, kv_pairs[i].value);
                if (written > 0 && (offset + written) < sizeof(kv_buffer))
                {
                    offset += written;
                }
            }
        }
    }

    /* 构造完整消息 */
    snprintf(full_message, sizeof(full_message), "%s%s", message, kv_buffer);

    /* 采样和过滤检查 */
    if (!should_log_message(full_message))
        return;

    /* 获取时间戳 */
    get_timestamp(timestamp, sizeof(timestamp));

    /* 加锁 */
    pthread_mutex_lock(&g_log_mutex);

    /* 检查并轮转日志文件 */
    check_and_rotate_log();

    /* 输出到终端（带颜色） */
    printf("%s[%s] [%s] [%s]%s %s\n",
           log_level_colors[level],
           timestamp,
           log_level_names[level],
           module < LOG_MODULE_MAX ? log_module_names[module] : "UNKNOWN",
           color_reset,
           full_message);

    /* 输出到文件（无颜色） */
    if (NULL != g_log_file)
    {
        fprintf(g_log_file, "[%s] [%s] [%s] %s\n",
               timestamp,
               log_level_names[level],
               module < LOG_MODULE_MAX ? log_module_names[module] : "UNKNOWN",
               full_message);
        fflush(g_log_file);
    }

    /* 发送到远程服务器 */
    char remote_log[OSAL_LOG_MESSAGE_SIZE + 256];
    snprintf(remote_log, sizeof(remote_log),
             "[%s] [%s] [%s] %s",
             timestamp,
             log_level_names[level],
             module < LOG_MODULE_MAX ? log_module_names[module] : "UNKNOWN",
             full_message);
    send_remote_log(remote_log);

    /* 解锁 */
    pthread_mutex_unlock(&g_log_mutex);
}
