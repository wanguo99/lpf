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
 *
 * 重构说明（v2.0）：
 * - 统一日志函数：使用单一的 OSAL_log_emit() 替代5个独立函数
 * - 编译时优化：支持编译时日志级别过滤（零开销）
 * - 运行时快速路径：使用原子变量进行无锁级别检查
 * - 延迟格式化：只有通过级别检查的日志才进行格式化
 * - 参考设计：Linux kernel printk + spdlog + zlog
 *
 * 性能优化：
 * - 快速路径 ~50ns（级别检查后返回，1GHz ARM）
 * - 无锁读取：使用 C11 _Atomic 和 __atomic_load_n
 * - 单次锁获取：格式化后才获取锁，减少临界区时间
 ************************************************************************/

#include "osal.h"
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
#include <stdatomic.h>  /* C11 原子操作支持 */

/*
 * 日志级别（使用 _Atomic 以支持无锁快速路径）
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
 *
 * 线程安全说明：
 * - g_log_level: 使用原子变量（_Atomic），支持无锁快速读取
 * - g_log_mutex: 保护日志文件操作和输出
 * - g_config_mutex: 保护配置变量（过滤器、远程日志等）
 * - 统计计数器使用原子操作（通过 __sync_* 内建函数）
 *
 * 性能优化：
 * - 级别检查使用 __atomic_load_explicit(..., __ATOMIC_RELAXED)
 * - 快速路径不持有任何锁，只读取原子变量
 * - 参考 Linux kernel printk 和 spdlog 的设计
 */
static _Atomic log_level_t g_log_level = LOG_LEVEL_INFO;  /* 使用原子类型 */
static log_level_t g_module_levels[LOG_MODULE_MAX];  /* 模块级日志级别 */
static bool g_module_level_set[LOG_MODULE_MAX] = {false};  /* 是否设置了模块级别 */
static FILE *g_log_file = NULL;
static osal_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;  /* 保护文件操作 */
static osal_mutex_t g_config_mutex = PTHREAD_MUTEX_INITIALIZER;  /* 保护配置变量 */
static char g_log_file_path[OSAL_LOG_PATH_SIZE] = {0};
static uint32_t g_max_log_size = OSAL_LOG_FILE_MAX_SIZE_MB * 1024 * 1024;  /* 10MB */
static uint32_t g_max_log_files = OSAL_LOG_FILE_BACKUP_COUNT;

/* 日志过滤和采样 */
static regex_t g_log_filter_regex;
static bool g_log_filter_enabled = false;
static uint32_t g_log_sampling_rate = 1;  /* 1表示全部记录 */
static uint64_t g_log_counter = 0;  /* 使用原子操作访问 */

/* 远程日志 */
static int g_remote_log_socket = -1;
static struct sockaddr_in g_remote_log_addr;
static bool g_remote_log_enabled = false;

/* 统计信息 */
static uint64_t g_total_log_count = 0;  /* 使用原子操作访问 */
static uint64_t g_dropped_log_count = 0;  /* 使用原子操作访问 */

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
int32_t OSAL_log_init(const char *log_file_path, int32_t level)
{
    uint32_t i;

    /* 设置日志级别（使用原子操作） */
    if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_FATAL)
    {
        __atomic_store_n(&g_log_level, level, __ATOMIC_RELAXED);
    }

    /* 初始化模块级别为未设置 */
    pthread_mutex_lock(&g_config_mutex);
    for (i = 0; i < LOG_MODULE_MAX; i++)
    {
        g_module_level_set[i] = false;
        g_module_levels[i] = LOG_LEVEL_INFO;
    }
    pthread_mutex_unlock(&g_config_mutex);

    /* 文件操作需要 g_log_mutex 保护 */
    if (NULL != log_file_path)
    {
        pthread_mutex_lock(&g_log_mutex);
        strncpy(g_log_file_path, log_file_path, OSAL_sizeof(g_log_file_path) - 1);
        g_log_file_path[OSAL_sizeof(g_log_file_path) - 1] = '\0';
        g_log_file = fopen(log_file_path, "a");
        pthread_mutex_unlock(&g_log_mutex);

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
void OSAL_log_shutdown(void)
{
    pthread_mutex_lock(&g_log_mutex);
    if (NULL != g_log_file)
    {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    pthread_mutex_unlock(&g_log_mutex);

    /* 关闭远程日志 */
    pthread_mutex_lock(&g_config_mutex);
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
    pthread_mutex_unlock(&g_config_mutex);
}

/**
 * @brief 设置日志级别
 *
 * 使用原子操作，无需加锁，支持运行时动态调整日志级别
 */
void OSAL_log_set_level(int32_t level)
{
    if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_FATAL)
    {
        __atomic_store_n(&g_log_level, level, __ATOMIC_RELAXED);
    }
}

/**
 * @brief 设置模块日志级别
 */
void OSAL_log_set_module_level(log_module_t module, int32_t level)
{
    if (module < LOG_MODULE_MAX && level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_FATAL)
    {
        pthread_mutex_lock(&g_config_mutex);
        g_module_levels[module] = level;
        g_module_level_set[module] = true;
        pthread_mutex_unlock(&g_config_mutex);
    }
}

/**
 * @brief 获取模块日志级别
 */
int32_t OSAL_log_get_module_level(log_module_t module)
{
    int32_t level;

    pthread_mutex_lock(&g_config_mutex);
    if (module < LOG_MODULE_MAX && g_module_level_set[module])
    {
        level = g_module_levels[module];
    }
    else
    {
        level = g_log_level;  /* 返回全局级别 */
    }
    pthread_mutex_unlock(&g_config_mutex);

    return level;
}

/**
 * @brief 设置日志过滤器
 */
int32_t OSAL_log_set_filter(const char *pattern)
{
    int ret;
    regex_t new_regex;

    if (NULL == pattern)
    {
        /* 清除过滤器 */
        pthread_mutex_lock(&g_config_mutex);
        if (g_log_filter_enabled)
        {
            regfree(&g_log_filter_regex);
            g_log_filter_enabled = false;
        }
        pthread_mutex_unlock(&g_config_mutex);
        return OSAL_SUCCESS;
    }

    /* 先编译正则表达式（不持有锁） */
    ret = regcomp(&new_regex, pattern, REG_EXTENDED | REG_NOSUB);
    if (ret != 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 加锁替换旧的过滤器 */
    pthread_mutex_lock(&g_config_mutex);
    if (g_log_filter_enabled)
    {
        regfree(&g_log_filter_regex);
    }
    g_log_filter_regex = new_regex;
    g_log_filter_enabled = true;
    pthread_mutex_unlock(&g_config_mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 设置日志采样率
 */
void OSAL_log_set_sampling(uint32_t rate)
{
    if (rate > 0)
    {
        pthread_mutex_lock(&g_config_mutex);
        g_log_sampling_rate = rate;
        pthread_mutex_unlock(&g_config_mutex);
    }
}

/**
 * @brief 启用远程日志
 */
int32_t OSAL_log_set_remote(const char *host, uint16_t port)
{
    int sock;
    struct sockaddr_in addr;

    if (NULL == host || port == 0)
    {
        return OSAL_ERR_INVALID_SIZE;
    }

    /* 创建UDP socket（不持有锁） */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        return OSAL_ERR_GENERIC;
    }

    /* 配置远程地址 */
    memset(&addr, 0, OSAL_sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0)
    {
        close(sock);
        return OSAL_ERR_GENERIC;
    }

    /* 加锁更新全局变量 */
    pthread_mutex_lock(&g_config_mutex);

    /* 关闭旧的 socket */
    if (g_remote_log_enabled && g_remote_log_socket >= 0)
    {
        close(g_remote_log_socket);
    }

    g_remote_log_socket = sock;
    g_remote_log_addr = addr;
    g_remote_log_enabled = true;

    pthread_mutex_unlock(&g_config_mutex);

    return OSAL_SUCCESS;
}

/**
 * @brief 禁用远程日志
 */
void OSAL_log_disable_remote(void)
{
    pthread_mutex_lock(&g_config_mutex);
    if (g_remote_log_enabled && g_remote_log_socket >= 0)
    {
        close(g_remote_log_socket);
        g_remote_log_socket = -1;
    }
    g_remote_log_enabled = false;
    pthread_mutex_unlock(&g_config_mutex);
}

/**
 * @brief 获取日志统计信息
 */
void OSAL_log_get_stats(uint64_t *total_count, uint64_t *dropped_count)
{
    if (total_count)
    {
        *total_count = __sync_fetch_and_add(&g_total_log_count, 0);
    }
    if (dropped_count)
    {
        *dropped_count = __sync_fetch_and_add(&g_dropped_log_count, 0);
    }
}

/**
 * @brief 设置日志文件最大大小
 */
void OSAL_log_set_max_file_size(uint32_t size_bytes)
{
    if (size_bytes > 0)
    {
        pthread_mutex_lock(&g_config_mutex);
        g_max_log_size = size_bytes;
        pthread_mutex_unlock(&g_config_mutex);
    }
}

/**
 * @brief 设置最大日志文件数
 */
void OSAL_log_set_max_files(uint32_t max_files)
{
    if (max_files > 0)
    {
        pthread_mutex_lock(&g_config_mutex);
        g_max_log_files = max_files;
        pthread_mutex_unlock(&g_config_mutex);
    }
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
 *
 * 注意：调用此函数前必须持有 g_log_mutex
 */
static void rotate_log_file(void)
{
    char old_file[OSAL_LOG_FILENAME_SIZE];
    char from[OSAL_LOG_FILENAME_SIZE], to[OSAL_LOG_FILENAME_SIZE];
    char current_backup[OSAL_LOG_FILENAME_SIZE];
    uint32_t i;
    uint32_t max_files;

    if (g_log_file_path[0] == '\0')
        return;

    /* 读取配置（需要临时获取 config_mutex） */
    pthread_mutex_unlock(&g_log_mutex);
    pthread_mutex_lock(&g_config_mutex);
    max_files = g_max_log_files;
    pthread_mutex_unlock(&g_config_mutex);
    pthread_mutex_lock(&g_log_mutex);

    /* 关闭当前日志文件 */
    if (NULL != g_log_file)
    {
        fclose(g_log_file);
        g_log_file = NULL;
    }

    /* 删除最旧的日志文件 */
    snprintf(old_file, OSAL_sizeof(old_file), "%s.%u", g_log_file_path, max_files);
    if (remove(old_file) != 0 && errno != ENOENT)
    {
        /* 文件不存在是正常情况，其他错误记录到 stderr */
        fprintf(stderr, "[LOG] 警告：无法删除旧日志文件 %s: %s\n", old_file, strerror(errno));
    }

    /* 重命名日志文件 */
    for (i = max_files - 1; i > 0; i--)
    {
        snprintf(from, OSAL_sizeof(from), "%s.%u", g_log_file_path, i - 1);
        snprintf(to, OSAL_sizeof(to), "%s.%u", g_log_file_path, i);
        if (rename(from, to) != 0 && errno != ENOENT)
        {
            fprintf(stderr, "[LOG] 警告：无法重命名日志文件 %s -> %s: %s\n",
                    from, to, strerror(errno));
        }
    }

    /* 重命名当前日志文件 */
    snprintf(current_backup, OSAL_sizeof(current_backup), "%s.1", g_log_file_path);
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
 *
 * 注意：调用此函数前必须持有 g_log_mutex
 */
static void check_and_rotate_log(void)
{
    int64_t file_size;
    uint32_t max_size;

    if (NULL == g_log_file || g_log_file_path[0] == '\0')
        return;

    /* 获取文件大小 */
    fseek(g_log_file, 0, SEEK_END);
    file_size = (int64_t)ftell(g_log_file);

    /* 读取配置（需要临时获取 config_mutex） */
    pthread_mutex_unlock(&g_log_mutex);
    pthread_mutex_lock(&g_config_mutex);
    max_size = g_max_log_size;
    pthread_mutex_unlock(&g_config_mutex);
    pthread_mutex_lock(&g_log_mutex);

    if (file_size > (int64_t)max_size)
    {
        rotate_log_file();
    }
}

/**
 * @brief 检查日志是否应该被记录（采样和过滤）
 *
 * 注意：使用原子操作更新计数器，使用 config_mutex 保护配置读取
 */
static bool should_log_message(const char *message)
{
    uint64_t counter;
    uint32_t sampling_rate;
    bool filter_enabled;

    /* 原子递增计数器 */
    counter = __sync_add_and_fetch(&g_log_counter, 1);

    /* 读取采样率配置 */
    pthread_mutex_lock(&g_config_mutex);
    sampling_rate = g_log_sampling_rate;
    filter_enabled = g_log_filter_enabled;

    /* 采样检查 */
    if (sampling_rate > 1 && (counter % sampling_rate) != 0)
    {
        pthread_mutex_unlock(&g_config_mutex);
        __sync_add_and_fetch(&g_dropped_log_count, 1);
        return false;
    }

    /* 过滤器检查 */
    if (filter_enabled && message != NULL)
    {
        if (regexec(&g_log_filter_regex, message, 0, NULL, 0) != 0)
        {
            pthread_mutex_unlock(&g_config_mutex);
            __sync_add_and_fetch(&g_dropped_log_count, 1);
            return false;
        }
    }

    pthread_mutex_unlock(&g_config_mutex);

    __sync_add_and_fetch(&g_total_log_count, 1);
    return true;
}

/**
 * @brief 发送日志到远程服务器
 *
 * 注意：使用 config_mutex 保护远程日志配置的读取
 */
static void send_remote_log(const char *log_message)
{
    int sock;
    struct sockaddr_in addr;
    bool enabled;

    if (log_message == NULL)
    {
        return;
    }

    /* 读取远程日志配置 */
    pthread_mutex_lock(&g_config_mutex);
    enabled = g_remote_log_enabled;
    sock = g_remote_log_socket;
    addr = g_remote_log_addr;
    pthread_mutex_unlock(&g_config_mutex);

    if (!enabled || sock < 0)
    {
        return;
    }

    /* 发送日志（不持有锁，避免阻塞） */
    sendto(sock, log_message, strlen(log_message), 0,
           (struct sockaddr *)&addr, OSAL_sizeof(addr));
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
 *
 * 性能优化：
 * 1. 快速路径：使用原子操作读取日志级别，无锁开销
 * 2. 延迟格式化：只有通过级别检查才进行 vsnprintf
 * 3. 单次锁获取：格式化后获取锁，减少临界区时间
 *
 * 参考 spdlog 的两级过滤设计
 */
static void log_internal_ex(log_level_t level, const char *module,
                            const char *file, const char *func, int32_t line,
                            const char *format, va_list args)
{
    char timestamp[OSAL_LOG_TIMESTAMP_SIZE];
    char message[OSAL_LOG_MESSAGE_SIZE];
    char full_log[OSAL_LOG_MESSAGE_SIZE + 256];
    const char *filename = extract_filename(file);
    log_level_t current_level;

    /* 快速路径：无锁读取日志级别（使用原子操作） */
    current_level = __atomic_load_n(&g_log_level, __ATOMIC_RELAXED);

    /* 第一级过滤：检查日志级别（快速返回） */
    if (level < current_level)
        return;

    /* 获取时间戳 */
    get_timestamp(timestamp, OSAL_sizeof(timestamp));

    /* 延迟格式化：只有通过级别检查才进行格式化 */
    vsnprintf(message, OSAL_sizeof(message), format, args);

    /* 采样和过滤检查 */
    if (!should_log_message(message))
        return;

    /* 加锁（临界区：文件操作和控制台输出） */
    pthread_mutex_lock(&g_log_mutex);

    /* 检查并轮转日志文件 */
    check_and_rotate_log();

    /* 构造完整日志 */
    snprintf(full_log, OSAL_sizeof(full_log),
             "[%s] [%s] [%s] [%s:%s:%d] %s",
             timestamp, log_level_names[level], module,
             filename, func, line, message);

    /* 输出到终端（带颜色） - 使用 write 系统调用 */
    {
        char console_buf[OSAL_LOG_MESSAGE_SIZE + 512];
        int console_len = snprintf(console_buf, OSAL_sizeof(console_buf),
                                   "%s[%s] [%s] [%s] [%s:%s:%d]%s %s\n",
                                   log_level_colors[level],
                                   timestamp,
                                   log_level_names[level],
                                   module,
                                   filename,
                                   func,
                                   line,
                                   color_reset,
                                   message);
        if (console_len > 0) {
            osal_ssize_t ret = write(STDOUT_FILENO, console_buf, (osal_size_t)console_len);
            if (ret < 0) {
                /* Write failed, but we can't log it (would cause recursion) */
            }
        }
    }

    /* 输出到文件（无颜色） */
    if (NULL != g_log_file)
    {
        fprintf(g_log_file, "%s\n", full_log);
        fflush(g_log_file);
    }

    /* 解锁 */
    pthread_mutex_unlock(&g_log_mutex);

    /* 发送到远程服务器（不持有锁，避免网络延迟阻塞日志系统） */
    send_remote_log(full_log);
}

/**
 * @brief 内部日志函数（不带位置信息，兼容旧接口）
 *
 * 性能优化：使用原子操作进行快速路径级别检查
 */
static void log_internal(log_level_t level, const char *module,
                         const char *format, va_list args)
{
    char timestamp[OSAL_LOG_TIMESTAMP_SIZE];
    char message[OSAL_LOG_MESSAGE_SIZE];
    log_level_t current_level;

    /* 快速路径：无锁读取日志级别 */
    current_level = __atomic_load_n(&g_log_level, __ATOMIC_RELAXED);

    /* 检查日志级别 */
    if (level < current_level)
        return;

    /* 获取时间戳 */
    get_timestamp(timestamp, OSAL_sizeof(timestamp));

    /* 格式化消息 */
    vsnprintf(message, OSAL_sizeof(message), format, args);

    /* 加锁 */
    pthread_mutex_lock(&g_log_mutex);

    /* 检查并轮转日志文件 */
    check_and_rotate_log();

    /* 输出到终端（带颜色） - 使用 write 系统调用 */
    if (NULL != module)
    {
        char console_buf[OSAL_LOG_MESSAGE_SIZE + 256];
        int console_len = snprintf(console_buf, OSAL_sizeof(console_buf),
                                   "%s[%s] [%s] [%s]%s %s\n",
                                   log_level_colors[level],
                                   timestamp,
                                   log_level_names[level],
                                   module,
                                   color_reset,
                                   message);
        if (console_len > 0) {
            osal_ssize_t ret = write(STDOUT_FILENO, console_buf, (osal_size_t)console_len);
            if (ret < 0) {
                /* Write failed, but we can't log it (would cause recursion) */
            }
        }
    }
    else
    {
        char console_buf[OSAL_LOG_MESSAGE_SIZE + 256];
        int console_len = snprintf(console_buf, OSAL_sizeof(console_buf),
                                   "%s[%s] [%s]%s %s\n",
                                   log_level_colors[level],
                                   timestamp,
                                   log_level_names[level],
                                   color_reset,
                                   message);
        if (console_len > 0) {
            osal_ssize_t ret = write(STDOUT_FILENO, console_buf, (osal_size_t)console_len);
            if (ret < 0) {
                /* Write failed, but we can't log it (would cause recursion) */
            }
        }
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
 * @brief 通用日志函数（兼容旧接口）
 */
void OSAL_log(int32_t level, const char *module, const char *format, ...)
{
    va_list args;

    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
        return;

    va_start(args, format);
    log_internal(level, module, format, args);
    va_end(args);
}

/**
 * @brief 统一的日志实现函数
 *
 * 所有日志宏（LOG_DEBUG/INFO/WARN/ERROR/FATAL）最终调用此函数
 *
 * 设计说明：
 * 1. 单一入口点，消除代码重复（原来5个独立函数共250行重复代码）
 * 2. 运行时快速路径：级别检查使用原子操作（__atomic_load）
 * 3. 延迟格式化：只有通过级别检查的日志才进行格式化
 * 4. 参考 Linux kernel vprintk_emit 和 spdlog 设计
 *
 * @param[in] level 日志级别（OS_LOG_LEVEL_DEBUG ~ OS_LOG_LEVEL_FATAL）
 * @param[in] module 模块名称（字符串）
 * @param[in] file 源文件名（__FILE__）
 * @param[in] func 函数名（__func__）
 * @param[in] line 行号（__LINE__）
 * @param[in] format 格式化字符串（printf 风格）
 * @param[in] ... 可变参数
 */
void OSAL_log_emit(int32_t level, const char *module,
                  const char *file, const char *func, int32_t line,
                  const char *format, ...)
{
    va_list args;

    /* 参数合法性检查 */
    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
        return;

    va_start(args, format);
    log_internal_ex(level, module, file, func, line, format, args);
    va_end(args);
}

/**
 * @brief 简单打印（不带日志级别和模块名）
 */
void OSAL_printf(const char *format, ...)
{
    char buffer[OSAL_LOG_MESSAGE_SIZE];
    va_list args;
    int len;

    va_start(args, format);
    len = vsnprintf(buffer, OSAL_sizeof(buffer), format, args);
    va_end(args);

    if (len > 0) {
        osal_ssize_t ret = write(STDOUT_FILENO, buffer, (osal_size_t)len);
        if (ret < 0) {
            /* Write failed, but we can't log it (would cause recursion) */
        }
    }
}

/**
 * @brief 结构化日志函数
 */
void OSAL_log_structured(int32_t level, log_module_t module, const char *message,
                        const log_kv_pair_t *kv_pairs, uint32_t kv_count)
{
    char timestamp[OSAL_LOG_TIMESTAMP_SIZE];
    char full_message[OSAL_LOG_MESSAGE_SIZE];
    char kv_buffer[512];
    char remote_log[OSAL_LOG_MESSAGE_SIZE + 256];
    uint32_t i;
    osal_size_t offset = 0;
    log_level_t current_level;
    log_level_t module_level;
    bool module_level_set;

    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
        return;

    /* 读取日志级别配置 */
    pthread_mutex_lock(&g_config_mutex);
    current_level = g_log_level;
    if (module < LOG_MODULE_MAX)
    {
        module_level_set = g_module_level_set[module];
        module_level = g_module_levels[module];
    }
    else
    {
        module_level_set = false;
        module_level = LOG_LEVEL_INFO;
    }
    pthread_mutex_unlock(&g_config_mutex);

    /* 检查模块级别 */
    if (module < LOG_MODULE_MAX && module_level_set)
    {
        if (level < module_level)
            return;
    }
    else if (level < current_level)
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
                int written = snprintf(kv_buffer + offset, OSAL_sizeof(kv_buffer) - offset,
                                      " %s=%s", kv_pairs[i].key, kv_pairs[i].value);
                if (written > 0 && (offset + (osal_size_t)written) < OSAL_sizeof(kv_buffer))
                {
                    offset += (osal_size_t)written;
                }
            }
        }
    }

    /* 构造完整消息 */
    snprintf(full_message, OSAL_sizeof(full_message), "%s%s", message, kv_buffer);

    /* 采样和过滤检查 */
    if (!should_log_message(full_message))
        return;

    /* 获取时间戳 */
    get_timestamp(timestamp, OSAL_sizeof(timestamp));

    /* 加锁 */
    pthread_mutex_lock(&g_log_mutex);

    /* 检查并轮转日志文件 */
    check_and_rotate_log();

    /* 输出到终端（带颜色） - 使用 write 系统调用 */
    {
        char console_buf[OSAL_LOG_MESSAGE_SIZE + 256];
        int console_len = snprintf(console_buf, OSAL_sizeof(console_buf),
                                   "%s[%s] [%s] [%s]%s %s\n",
                                   log_level_colors[level],
                                   timestamp,
                                   log_level_names[level],
                                   module < LOG_MODULE_MAX ? log_module_names[module] : "UNKNOWN",
                                   color_reset,
                                   full_message);
        if (console_len > 0) {
            osal_ssize_t ret = write(STDOUT_FILENO, console_buf, (osal_size_t)console_len);
            if (ret < 0) {
                /* Write failed, but we can't log it (would cause recursion) */
            }
        }
    }

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

    /* 解锁 */
    pthread_mutex_unlock(&g_log_mutex);

    /* 发送到远程服务器（不持有锁） */
    snprintf(remote_log, OSAL_sizeof(remote_log),
             "[%s] [%s] [%s] %s",
             timestamp,
             log_level_names[level],
             module < LOG_MODULE_MAX ? log_module_names[module] : "UNKNOWN",
             full_message);
    send_remote_log(remote_log);
}
