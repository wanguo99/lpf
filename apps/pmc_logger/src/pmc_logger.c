#include "pmc_logger.h"
#include "libpmc_ipc.h"
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

/* 日志配置 */
#define LOG_DIR "/var/log/pmc"
#define LOG_FILE_PREFIX "pmc"
#define LOG_FILE_MAX_SIZE (10 * 1024 * 1024)  /* 10MB */
#define LOG_BUFFER_SIZE 1024

/* 全局变量 */
static FILE *g_log_file = NULL;
static uint32 g_log_file_size = 0;
static pmc_process_heartbeat_t *g_heartbeat = NULL;

/* 获取当前时间字符串 */
static void get_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* 获取日志文件名 */
static void get_log_filename(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    snprintf(buffer, size, "%s/%s_%04d%02d%02d.log",
             LOG_DIR, LOG_FILE_PREFIX,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday);
}

/* 打开日志文件 */
static int32 open_log_file(void)
{
    char filename[256];
    struct stat st;

    /* 创建日志目录 */
    if (stat(LOG_DIR, &st) != 0) {
        mkdir(LOG_DIR, 0755);
    }

    /* 获取日志文件名 */
    get_log_filename(filename, sizeof(filename));

    /* 打开日志文件（追加模式） */
    g_log_file = fopen(filename, "a");
    if (g_log_file == NULL) {
        LOG_ERROR("LOGGER", "打开日志文件失败: %s", filename);
        return OS_ERROR;
    }

    /* 获取文件大小 */
    fseek(g_log_file, 0, SEEK_END);
    g_log_file_size = ftell(g_log_file);

    LOG_INFO("LOGGER", "打开日志文件: %s (size=%u)", filename, g_log_file_size);
    return OS_SUCCESS;
}

/* 关闭日志文件 */
static void close_log_file(void)
{
    if (g_log_file) {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = NULL;
        g_log_file_size = 0;
    }
}

/* 轮转日志文件 */
static int32 rotate_log_file(void)
{
    LOG_INFO("LOGGER", "轮转日志文件 (size=%u)", g_log_file_size);

    /* 关闭当前文件 */
    close_log_file();

    /* 打开新文件 */
    return open_log_file();
}

/* 写入日志 */
static int32 write_log(const pmc_log_entry_t *entry)
{
    char timestamp[64];
    const char *level_str;

    if (g_log_file == NULL) {
        return OS_ERROR;
    }

    /* 获取时间戳 */
    get_timestamp(timestamp, sizeof(timestamp));

    /* 获取日志级别字符串 */
    switch (entry->level) {
        case PMC_LOG_DEBUG:   level_str = "DEBUG"; break;
        case PMC_LOG_INFO:    level_str = "INFO "; break;
        case PMC_LOG_WARN:    level_str = "WARN "; break;
        case PMC_LOG_ERROR:   level_str = "ERROR"; break;
        default:              level_str = "UNKN "; break;
    }

    /* 写入日志 */
    int32 len = fprintf(g_log_file, "[%s] [%s] [%s] %s\n",
                        timestamp, level_str, entry->module, entry->message);
    if (len < 0) {
        return OS_ERROR;
    }

    g_log_file_size += len;

    /* 检查是否需要轮转 */
    if (g_log_file_size >= LOG_FILE_MAX_SIZE) {
        rotate_log_file();
    }

    return OS_SUCCESS;
}

/* 日志收集任务 */
static void log_collector_task(void *arg)
{
    pmc_log_entry_t entry;
    int32 ret;

    (void)arg;

    LOG_INFO("LOGGER", "日志收集任务启动");

    while (!OSAL_TaskShouldShutdown()) {
        /* 从共享内存读取日志 */
        ret = PMC_Log_Read(&entry);
        if (ret == OS_SUCCESS) {
            /* 写入日志文件 */
            write_log(&entry);
        } else {
            /* 没有日志，休眠一会 */
            OSAL_TaskDelay(100);
        }

        /* 定期刷新文件 */
        static uint32 flush_counter = 0;
        if (++flush_counter >= 10) {
            if (g_log_file) {
                fflush(g_log_file);
            }
            flush_counter = 0;
        }
    }

    LOG_INFO("LOGGER", "日志收集任务退出");
}

/* 心跳任务 */
static void heartbeat_task(void *arg)
{
    (void)arg;

    LOG_INFO("LOGGER", "心跳任务启动");

    while (!OSAL_TaskShouldShutdown()) {
        /* 更新心跳 */
        PMC_Heartbeat_Update(g_heartbeat, PMC_PROCESS_LOGGER);

        /* 休眠500ms */
        OSAL_TaskDelay(500);
    }

    LOG_INFO("LOGGER", "心跳任务退出");
}

/* 初始化 */
int32 PMC_Logger_Init(void)
{
    int32 ret;

    LOG_INFO("LOGGER", "Logger进程初始化...");

    /* 打开日志文件 */
    ret = open_log_file();
    if (ret != OS_SUCCESS) {
        LOG_ERROR("LOGGER", "打开日志文件失败: %d", ret);
        return ret;
    }

    /* 初始化日志IPC */
    ret = PMC_Log_Init();
    if (ret != OS_SUCCESS) {
        LOG_ERROR("LOGGER", "初始化日志IPC失败: %d", ret);
        close_log_file();
        return ret;
    }

    /* 初始化心跳 */
    ret = PMC_Heartbeat_Init(&g_heartbeat);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("LOGGER", "初始化心跳失败: %d", ret);
        PMC_Log_Cleanup();
        close_log_file();
        return ret;
    }

    LOG_INFO("LOGGER", "Logger进程初始化完成");
    return OS_SUCCESS;
}

/* 主循环 */
int32 PMC_Logger_Run(void)
{
    int32 ret;
    osal_id_t log_task_id, heartbeat_task_id;

    LOG_INFO("LOGGER", "Logger进程开始运行");

    /* 创建日志收集任务 */
    ret = OSAL_TaskCreate(&log_task_id, "log_collector",
                          log_collector_task, NULL,
                          8192, OSAL_PRIORITY_NORMAL, 0);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("LOGGER", "创建日志收集任务失败: %d", ret);
        return ret;
    }

    /* 创建心跳任务 */
    ret = OSAL_TaskCreate(&heartbeat_task_id, "heartbeat",
                          heartbeat_task, NULL,
                          4096, OSAL_PRIORITY_NORMAL, 0);
    if (ret != OS_SUCCESS) {
        LOG_ERROR("LOGGER", "创建心跳任务失败: %d", ret);
        OSAL_TaskDelete(log_task_id);
        return ret;
    }

    /* 等待任务退出 */
    OSAL_TaskWaitForShutdown();

    /* 等待任务结束 */
    OSAL_TaskDelay(1000);

    LOG_INFO("LOGGER", "Logger进程退出");
    return OS_SUCCESS;
}

/* 清理 */
void PMC_Logger_Cleanup(void)
{
    LOG_INFO("LOGGER", "Logger进程清理...");

    if (g_heartbeat) {
        PMC_Heartbeat_Cleanup(g_heartbeat);
        g_heartbeat = NULL;
    }

    PMC_Log_Cleanup();
    close_log_file();

    LOG_INFO("LOGGER", "Logger进程清理完成");
}
