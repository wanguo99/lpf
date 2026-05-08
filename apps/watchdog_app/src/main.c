/************************************************************************
 * Watchdog应用 - 主程序
 *
 * 这是一个看门狗喂狗应用，展示如何使用HAL层的Watchdog接口：
 * - 初始化看门狗设备
 * - 定期喂狗
 * - 监控喂狗统计信息
 * - 优雅退出
 ************************************************************************/

#include "osal.h"
#include "hal_watchdog.h"
#include <stdatomic.h>

/* 应用版本 */
#define APP_VERSION "1.0.0"

/* 喂狗间隔（毫秒） */
#define WATCHDOG_KICK_INTERVAL_MS  5000   /* 5秒喂一次狗 */

/* 统计信息打印间隔（毫秒） */
#define STATS_INTERVAL_MS          30000  /* 30秒打印一次统计 */

/* 全局运行标志 */
static volatile bool g_running = true;

/* 看门狗句柄 */
static hal_watchdog_handle_t g_watchdog_handle = NULL;

/* 线程句柄 */
static osal_thread_t g_watchdog_thread = 0;
static osal_thread_t g_stats_thread = 0;

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
 * @brief 看门狗喂狗任务
 *
 * 定期喂狗，防止系统重启
 */
static void *watchdog_task(void *arg)
{
    (void)arg;

    LOG_INFO("Watchdog", "喂狗线程启动");

    while (g_running)
    {
        /* 喂狗 */
        int32_t ret = HAL_WATCHDOG_Kick(g_watchdog_handle);
        if (ret == OSAL_SUCCESS)
        {
            LOG_INFO("Watchdog", "喂狗成功");
        }
        else
        {
            LOG_ERROR("Watchdog", "喂狗失败: %d", ret);
        }

        /* 延时 */
        OSAL_msleep(WATCHDOG_KICK_INTERVAL_MS);
    }

    LOG_INFO("Watchdog", "喂狗线程退出");
    return NULL;
}

/**
 * @brief 统计任务
 *
 * 定期打印看门狗统计信息
 */
static void *stats_task(void *arg)
{
    (void)arg;

    LOG_INFO("Stats", "统计线程启动");

    while (g_running)
    {
        OSAL_msleep(STATS_INTERVAL_MS);

        if (!g_running)
        {
            break;
        }

        /* 获取统计信息 */
        uint32_t kick_count = 0;
        int32_t ret = HAL_WATCHDOG_GetStats(g_watchdog_handle, &kick_count);
        if (ret == OSAL_SUCCESS)
        {
            LOG_INFO("Stats", "喂狗次数: %u", kick_count);
        }

        /* 获取超时时间 */
        uint32_t timeout = 0;
        ret = HAL_WATCHDOG_GetTimeout(g_watchdog_handle, &timeout);
        if (ret == OSAL_SUCCESS)
        {
            LOG_INFO("Stats", "超时时间: %u 秒", timeout);
        }

        /* 获取剩余时间（如果硬件支持） */
        uint32_t timeleft = 0;
        ret = HAL_WATCHDOG_GetTimeleft(g_watchdog_handle, &timeleft);
        if (ret == OSAL_SUCCESS)
        {
            LOG_INFO("Stats", "剩余时间: %u 秒", timeleft);
        }
    }

    LOG_INFO("Stats", "统计线程退出");
    return NULL;
}

/**
 * @brief 主函数
 */
int32_t main(int32_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    OSAL_Printf("========================================\n");
    OSAL_Printf("Watchdog应用 v%s\n", APP_VERSION);
    OSAL_Printf("========================================\n\n");

    /* 注册信号处理 */
    OSAL_SignalRegister(OS_SIGNAL_INT, signal_handler);
    OSAL_SignalRegister(OS_SIGNAL_TERM, signal_handler);

    /* 初始化看门狗 */
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 60,          /* 60秒超时 */
        .enable_on_init = true      /* 初始化时启用 */
    };

    LOG_INFO("Main", "初始化看门狗设备: %s", config.device);
    int32_t ret = HAL_WATCHDOG_Init(&config, &g_watchdog_handle);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("Main", "看门狗初始化失败: %d", ret);
        LOG_ERROR("Main", "请确保:");
        LOG_ERROR("Main", "  1. 看门狗设备存在: %s", config.device);
        LOG_ERROR("Main", "  2. 有足够的权限访问设备（可能需要root权限）");
        LOG_ERROR("Main", "  3. 看门狗驱动已加载");
        return OSAL_ERR_GENERIC;
    }

    LOG_INFO("Main", "看门狗初始化成功");

    /* 创建喂狗线程 */
    ret = OSAL_ThreadCreate(&g_watchdog_thread, watchdog_task, NULL);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("Main", "创建喂狗线程失败");
        HAL_WATCHDOG_Deinit(g_watchdog_handle);
        return OSAL_ERR_GENERIC;
    }

    /* 创建统计线程 */
    ret = OSAL_ThreadCreate(&g_stats_thread, stats_task, NULL);
    if (ret != OSAL_SUCCESS)
    {
        LOG_ERROR("Main", "创建统计线程失败");
        g_running = false;
        OSAL_ThreadJoin(g_watchdog_thread);
        HAL_WATCHDOG_Deinit(g_watchdog_handle);
        return OSAL_ERR_GENERIC;
    }

    LOG_INFO("Main", "应用启动成功，按Ctrl+C退出");
    OSAL_Printf("\n提示：\n");
    OSAL_Printf("  - 看门狗超时时间: %u 秒\n", config.timeout_sec);
    OSAL_Printf("  - 喂狗间隔: %u 毫秒\n", WATCHDOG_KICK_INTERVAL_MS);
    OSAL_Printf("  - 统计信息间隔: %u 毫秒\n\n", STATS_INTERVAL_MS);

    /* 等待线程退出 */
    OSAL_ThreadJoin(g_watchdog_thread);
    OSAL_ThreadJoin(g_stats_thread);

    /* 清理资源 */
    LOG_INFO("Main", "关闭看门狗设备");
    HAL_WATCHDOG_Deinit(g_watchdog_handle);

    OSAL_Printf("\n应用已退出\n");
    return OSAL_SUCCESS;
}
