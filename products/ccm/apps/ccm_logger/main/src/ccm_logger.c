#include "ccm_logger.h"
#include "libccm/libccm_ipc.h"
#include "osal/sys/osal_signal.h"

/* 全局变量 */
static pmc_log_ringbuffer_t *g_log_ring = NULL;
static pmc_process_heartbeat_t *g_heartbeat = NULL;
static volatile bool g_running = true;

/* 线程ID */
static osal_thread_t g_log_thread = 0;

/* 信号处理 */
static void signal_handler(int32_t sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        g_running = false;
        LOG_INFO("LOGGER", "收到退出信号");
    }
}


/* 日志收集线程 */
static void *log_collector_thread(void *arg)
{
    char log_entry[PMC_LOG_ENTRY_SIZE];
    int32_t ret;

    (void)arg;

    LOG_INFO("LOGGER", "日志收集线程启动");

    while (g_running) {
        /* 更新心跳 */
        PMC_Heartbeat_Update(g_heartbeat, PMC_PROCESS_LOGGER);

        /* 从共享内存读取日志 */
        ret = PMC_Log_Read(g_log_ring, log_entry, sizeof(log_entry));
        if (ret == OSAL_SUCCESS) {
            /* TODO: 写入日志文件或输出到控制台 */
            LOG_DEBUG("LOGGER", "收到日志: %s", log_entry);
        } else {
            /* 没有日志，休眠一会 */
            OSAL_msleep(100);
        }
    }

    LOG_INFO("LOGGER", "日志收集线程退出");
    return NULL;
}


/* 初始化 */
int32_t PMC_Logger_Init(void)
{
    int32_t ret;

    LOG_INFO("LOGGER", "Logger进程初始化...");

    /* 注册信号处理 */
    OSAL_SignalRegister(SIGTERM, signal_handler);
    OSAL_SignalRegister(SIGINT, signal_handler);

    /* 初始化日志环形缓冲区 */
    ret = PMC_Log_Init(&g_log_ring);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LOGGER", "初始化日志环形缓冲区失败: %d", ret);
        return ret;
    }

    /* 初始化心跳 */
    ret = PMC_Heartbeat_Init(&g_heartbeat);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LOGGER", "初始化心跳失败: %d", ret);
        PMC_Log_Cleanup(g_log_ring);
        return ret;
    }

    LOG_INFO("LOGGER", "Logger进程初始化完成");
    return OSAL_SUCCESS;
}


/* 主循环 */
int32_t PMC_Logger_Run(void)
{
    int32_t ret;

    LOG_INFO("LOGGER", "Logger进程开始运行");

    /* 创建日志收集线程 */
    ret = OSAL_ThreadCreate(&g_log_thread, log_collector_thread, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("LOGGER", "创建日志收集线程失败: %d", ret);
        return ret;
    }

    /* 等待退出信号 */
    while (g_running) {
        OSAL_msleep(1000);
    }

    /* 等待线程退出 */
    LOG_INFO("LOGGER", "等待线程退出...");
    OSAL_msleep(2000);

    LOG_INFO("LOGGER", "Logger进程退出");
    return OSAL_SUCCESS;
}


/* 清理 */
void PMC_Logger_Cleanup(void)
{
    LOG_INFO("LOGGER", "Logger进程清理...");

    if (g_heartbeat) {
        PMC_Heartbeat_Cleanup(g_heartbeat);
        g_heartbeat = NULL;
    }

    if (g_log_ring) {
        PMC_Log_Cleanup(g_log_ring);
        g_log_ring = NULL;
    }

    LOG_INFO("LOGGER", "Logger进程清理完成");
}
