/************************************************************************
 * Sample应用 - 主程序
 *
 * 这是一个最小化的示例应用，展示如何正确使用EMS各层接口：
 * - OSAL层：任务、队列、信号处理、日志
 * - HAL层：（可选）硬件驱动
 * - PDL层：（可选）外设服务
 *
 * 架构说明：
 * 1. 使用OSAL接口进行任务管理、日志输出
 * 2. 通过信号处理实现优雅退出
 * 3. 展示多任务协作模式
 * 4. 展示队列通信机制
 ************************************************************************/

#include "osal.h"
#include <stdatomic.h>  /* C11原子操作 */

/* 应用版本 */
#define APP_VERSION "1.0.0"

/* 任务配置 */
#define TASK_STACK_SIZE    (16 * 1024)  /* 16KB */
#define TASK_PRIORITY      100

/* 队列配置 */
#define QUEUE_DEPTH        10
#define QUEUE_MSG_SIZE     64

/* 定时器配置 */
#define HEARTBEAT_INTERVAL_MS  1000   /* 1秒 */
#define STATS_INTERVAL_MS      30000  /* 30秒 */

/* 全局运行标志 */
static volatile bool g_running = true;

/* 任务ID */
static osal_id_t g_worker_task_id = 0;
static osal_id_t g_stats_task_id = 0;

/* 消息队列ID */
static osal_id_t g_msg_queue_id = 0;

/* 统计计数器 */
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
 * - 任务循环检查退出标志
 * - 使用队列发送消息
 * - 使用原子操作更新计数器
 */
static void worker_task(void *arg)
{
    (void)arg;  /* 未使用的参数 */

    uint32_t counter = 0;
    int32_t ret;

    LOG_INFO("Worker", "工作任务启动");

    while (!OSAL_TaskShouldShutdown())
    {
        /* 构造消息 */
        char msg[QUEUE_MSG_SIZE];
        OSAL_Snprintf(msg, sizeof(msg), "Message #%u", counter++);

        /* 发送消息到队列 */
        ret = OSAL_QueuePut(g_msg_queue_id, msg, sizeof(msg), 1000);
        if (OSAL_SUCCESS == ret)
        {
            atomic_fetch_add(&g_msg_count, 1);
            LOG_INFO("Worker", "发送消息: %s", msg);
        }
        else if (OSAL_ERR_QUEUE_TIMEOUT == ret)
        {
            LOG_WARN("Worker", "队列发送超时");
        }
        else
        {
            LOG_ERROR("Worker", "队列发送失败: %d", ret);
        }

        /* 延时1秒 */
        OSAL_TaskDelay(HEARTBEAT_INTERVAL_MS);
    }

    LOG_INFO("Worker", "工作任务退出");
}

/**
 * @brief 统计任务
 *
 * 演示：
 * - 定期打印统计信息
 * - 从队列接收消息
 */
static void stats_task(void *arg)
{
    (void)arg;  /* 未使用的参数 */

    int32_t ret;
    int8_t msg[QUEUE_MSG_SIZE];
    uint32_t msg_size;

    LOG_INFO("Stats", "统计任务启动");

    while (!OSAL_TaskShouldShutdown())
    {
        /* 从队列接收消息（超时5秒） */
        ret = OSAL_QueueGet(g_msg_queue_id, msg, sizeof(msg), &msg_size, 5000);
        if (OSAL_SUCCESS == ret)
        {
            LOG_INFO("Stats", "接收消息: %s (大小: %u字节)", msg, msg_size);
        }
        else if (OSAL_ERR_QUEUE_TIMEOUT == ret)
        {
            /* 超时是正常的，打印统计信息 */
            uint32_t count = atomic_load(&g_msg_count);
            OSAL_Printf("\n========== 应用统计 ==========\n");
            OSAL_Printf("已处理消息数: %u\n", count);
            OSAL_Printf("==============================\n\n");
        }
        else
        {
            LOG_ERROR("Stats", "队列接收失败: %d", ret);
            break;
        }
    }

    LOG_INFO("Stats", "统计任务退出");
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

    /* 2. 创建消息队列 */
    ret = OSAL_QueueCreate(&g_msg_queue_id, "MsgQueue",
                           QUEUE_DEPTH, QUEUE_MSG_SIZE, 0);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("Main", "创建消息队列失败: %d", ret);
        goto cleanup;
    }
    LOG_INFO("Main", "消息队列创建成功 (深度: %d, 消息大小: %d字节)",
              QUEUE_DEPTH, QUEUE_MSG_SIZE);

    /* 3. 创建工作任务 */
    ret = OSAL_TaskCreate(&g_worker_task_id, "WorkerTask",
                          worker_task, NULL,
                          TASK_STACK_SIZE, TASK_PRIORITY, 0);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("Main", "创建工作任务失败: %d", ret);
        goto cleanup;
    }
    LOG_INFO("Main", "工作任务创建成功");

    /* 4. 创建统计任务 */
    ret = OSAL_TaskCreate(&g_stats_task_id, "StatsTask",
                          stats_task, NULL,
                          TASK_STACK_SIZE, TASK_PRIORITY, 0);
    if (OSAL_SUCCESS != ret)
    {
        LOG_ERROR("Main", "创建统计任务失败: %d", ret);
        goto cleanup;
    }
    LOG_INFO("Main", "统计任务创建成功");

    OSAL_Printf("\n应用启动成功！按Ctrl+C退出。\n\n");

    /* 5. 主循环 - 等待退出信号 */
    while (g_running)
    {
        OSAL_TaskDelay(1000);  /* 1秒检查一次 */
    }

    OSAL_Printf("\n[Main] 开始清理资源...\n");

cleanup:
    /* 6. 删除任务 */
    if (0 != g_worker_task_id)
    {
        ret = OSAL_TaskDelete(g_worker_task_id);
        if (OSAL_SUCCESS == ret)
        {
            LOG_INFO("Main", "工作任务已删除");
        }
        else
        {
            LOG_ERROR("Main", "删除工作任务失败: %d", ret);
        }
    }

    if (0 != g_stats_task_id)
    {
        ret = OSAL_TaskDelete(g_stats_task_id);
        if (OSAL_SUCCESS == ret)
        {
            LOG_INFO("Main", "统计任务已删除");
        }
        else
        {
            LOG_ERROR("Main", "删除统计任务失败: %d", ret);
        }
    }

    /* 7. 删除队列 */
    if (0 != g_msg_queue_id)
    {
        ret = OSAL_QueueDelete(g_msg_queue_id);
        if (OSAL_SUCCESS == ret)
        {
            LOG_INFO("Main", "消息队列已删除");
        }
        else
        {
            LOG_ERROR("Main", "删除消息队列失败: %d", ret);
        }
    }

    OSAL_Printf("\n应用已退出。\n");

    return 0;
}
