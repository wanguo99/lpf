/************************************************************************
 * Sample应用 - 主程序
 *
 * 这是一个最小化的示例应用，展示如何正确使用EMS各层接口：
 * - OSAL层：线程、互斥锁、信号处理、日志
 * - HAL层：（可选）硬件驱动
 * - PDL层：（可选）外设服务
 *
 * 架构说明：
 * 1. 使用OSAL接口进行线程管理、日志输出
 * 2. 通过信号处理实现优雅退出
 * 3. 展示多线程协作模式
 * 4. 展示互斥锁保护共享资源
 ************************************************************************/

#include "osal.h"
#include <stdatomic.h>  /* C11原子操作 */

/* 应用版本 */
#define APP_VERSION "1.0.0"

/* 定时器配置 */
#define HEARTBEAT_INTERVAL_MS  1000   /* 1秒 */
#define STATS_INTERVAL_MS      5000   /* 5秒 */

/* 全局运行标志 */
static volatile bool g_running = true;

/* 线程句柄 */
static osal_thread_t g_worker_thread = 0;
static osal_thread_t g_stats_thread = 0;

/* 互斥锁指针 */
static osal_mutex_t *g_mutex = NULL;

/* 统计计数器（使用原子操作） */
static atomic_uint g_msg_count = 0;

/**
 * @brief 信号处理函数
 *
 * 捕获SIGINT(Ctrl+C)和SIGTERM信号，实现优雅退出
 */
static void signal_handler(int32_t sig)
{
    if (sig == OS_SIGNAL_INT || sig == OS_SIGNAL_TERM)
    {
        OSAL_Printf("\n[Main] 收到退出信号，正在关闭应用...\n");
        g_running = false;
    }
}

/**
 * @brief 工作任务
 *
 * 演示：
 * - 线程循环检查退出标志
 * - 使用互斥锁保护日志输出
 * - 使用原子操作更新计数器
 */
static void *worker_task(void *arg)
{
    (void)arg;  /* 未使用的参数 */

    uint32_t counter = 0;

    LOG_INFO("Worker", "工作线程启动");

    while (g_running)
    {
        /* 使用互斥锁保护日志输出 */
        OSAL_MutexLock(g_mutex);
        LOG_INFO("Worker", "心跳 #%u", counter);
        OSAL_MutexUnlock(g_mutex);

        /* 更新计数器 */
        atomic_fetch_add(&g_msg_count, 1);
        counter++;

        /* 延时1秒 */
        OSAL_msleep(HEARTBEAT_INTERVAL_MS);
    }

    LOG_INFO("Worker", "工作线程退出");
    return NULL;
}

/**
 * @brief 统计任务
 *
 * 演示：
 * - 定期打印统计信息
 * - 使用互斥锁保护输出
 */
static void *stats_task(void *arg)
{
    (void)arg;  /* 未使用的参数 */

    LOG_INFO("Stats", "统计线程启动");

    while (g_running)
    {
        /* 延时5秒 */
        OSAL_msleep(STATS_INTERVAL_MS);

        if (!g_running)
            break;

        /* 使用互斥锁保护统计输出 */
        OSAL_MutexLock(g_mutex);

        uint32_t count = atomic_load(&g_msg_count);
        OSAL_Printf("\n========== 应用统计 ==========\n");
        OSAL_Printf("工作线程心跳次数: %u\n", count);
        OSAL_Printf("==============================\n\n");

        OSAL_MutexUnlock(g_mutex);
    }

    LOG_INFO("Stats", "统计线程退出");
    return NULL;
}

/**
 * @brief 主函数
 */
int main(int32_t argc, char *argv[])
{
    int32_t ret;

    (void)argc;  /* 未使用的参数 */
    (void)argv;

    OSAL_Printf("========================================\n");
    OSAL_Printf("  Sample应用 v%s\n", APP_VERSION);
    OSAL_Printf("  OSAL版本: %s\n", OSAL_GetVersionString());
    OSAL_Printf("========================================\n\n");

    /* 注意：OSAL作为用户态库，不需要显式初始化 */

    /* 1. 注册信号处理 */
    ret = OSAL_SignalRegister(OS_SIGNAL_INT, signal_handler);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("Main", "注册SIGINT失败: %d", ret);
        goto cleanup;
    }

    ret = OSAL_SignalRegister(OS_SIGNAL_TERM, signal_handler);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("Main", "注册SIGTERM失败: %d", ret);
        goto cleanup;
    }
    LOG_INFO("Main", "信号处理注册成功");

    /* 2. 创建互斥锁 */
    ret = OSAL_MutexCreate(&g_mutex);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("Main", "创建互斥锁失败: %d", ret);
        goto cleanup;
    }
    LOG_INFO("Main", "互斥锁创建成功");

    /* 3. 创建工作线程 */
    ret = OSAL_ThreadCreate(&g_worker_thread, worker_task, NULL);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("Main", "创建工作线程失败: %d", ret);
        goto cleanup;
    }
    LOG_INFO("Main", "工作线程创建成功");

    /* 4. 创建统计线程 */
    ret = OSAL_ThreadCreate(&g_stats_thread, stats_task, NULL);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("Main", "创建统计线程失败: %d", ret);
        goto cleanup;
    }
    LOG_INFO("Main", "统计线程创建成功");

    OSAL_Printf("\n应用启动成功！按Ctrl+C退出。\n\n");

    /* 5. 主循环 - 等待退出信号 */
    while (g_running)
    {
        OSAL_msleep(1000);  /* 1秒检查一次 */
    }

    OSAL_Printf("\n[Main] 开始清理资源...\n");

cleanup:
    /* 6. 等待线程退出 */
    if (0 != g_worker_thread)
    {
        OSAL_ThreadJoin(g_worker_thread);
        LOG_INFO("Main", "工作线程已退出");
    }

    if (0 != g_stats_thread)
    {
        OSAL_ThreadJoin(g_stats_thread);
        LOG_INFO("Main", "统计线程已退出");
    }

    /* 7. 删除互斥锁 */
    if (NULL != g_mutex)
    {
        ret = OSAL_MutexDelete(g_mutex);
        if (OSAL_SUCCESS == ret)
        {
            LOG_INFO("Main", "互斥锁已删除");
        }
        else
        {
            LOG_ERROR("Main", "删除互斥锁失败: %d", ret);
        }
    }

    OSAL_Printf("\n应用已退出。\n");

    return 0;
}
