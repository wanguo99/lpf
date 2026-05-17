/**
 * @file telecommand_main.c
 * @brief Telecommand进程主程序
 * @note 实时进程，CPU0，优先级99
 *       职责：
 *       1. 接收CAN遥控命令（<20μs）
 *       2. 执行遥控操作（调用PDL）
 *       3. 处理遥测请求（从缓存读取，<50μs）
 *       4. 2ms内应答
 */

#include "osal.h"
#include "acl_api.h"
#include "acl_config.h"
#include "acl_telemetry_cache.h"
#include "pmc_acl_types.h"
#include "pdl_bmc.h"
#include "pdl_mcu.h"

/* 全局运行标志 */
static volatile bool g_running = true;

/* 信号处理 */
static void signal_handler(int32_t sig)
{
    (void)sig;
    g_running = false;
}

/**
 * @brief 设置实时调度策略
 */
static int32_t setup_realtime_scheduling(void)
{
    int32_t ret;

    /* 设置SCHED_FIFO调度策略，优先级99 */
    ret = OSAL_SchedSetPolicy(0, OSAL_SCHED_FIFO, 99);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("TC", "设置实时调度失败: %d", ret);
        return ret;
    }

    /* 绑定到CPU0 */
    ret = OSAL_SchedSetAffinity(0, 0);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("TC", "设置CPU亲和性失败: %d", ret);
        return ret;
    }

    /* 锁定内存，避免页面交换 */
    ret = OSAL_MemLock(true);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("TC", "锁定内存失败: %d", ret);
        return ret;
    }

    LOG_INFO("TC", "实时调度配置完成: SCHED_FIFO, priority=99, CPU=0");

    return OSAL_SUCCESS;
}

/**
 * @brief 处理遥控命令
 */
static int32_t handle_telecommand(pmc_tc_function_t cmd_type, uint32_t param)
{
    (void)param;

    /* 通过ACL查询设备映射 */
    const acl_tc_config_t *cfg = ACL_GetTcConfig(cmd_type);
    if (!cfg || !cfg->enabled) {
        LOG_WARN("TC", "遥控命令未配置: %d", cmd_type);
        return OSAL_ERR_GENERIC;
    }

    /* 根据设备类型调用PDL接口 */
    int32_t ret = OSAL_ERR_NOT_IMPLEMENTED;

    switch (cfg->device_type) {
        case ACL_DEVICE_BMC:
            /* TODO: 调用BMC PDL接口 */
            LOG_INFO("TC", "执行BMC遥控: %d", cmd_type);
            ret = OSAL_SUCCESS;
            break;

        case ACL_DEVICE_MCU:
            /* TODO: 调用MCU PDL接口 */
            LOG_INFO("TC", "执行MCU遥控: %d", cmd_type);
            ret = OSAL_SUCCESS;
            break;

        case ACL_DEVICE_FPGA:
            /* TODO: 调用FPGA PDL接口 */
            LOG_INFO("TC", "执行FPGA遥控: %d", cmd_type);
            ret = OSAL_SUCCESS;
            break;

        default:
            LOG_ERROR("TC", "未知设备类型: %d", cfg->device_type);
            return OSAL_ERR_NOT_IMPLEMENTED;
    }

    /* 遥控执行后，失效相关遥测缓存 */
    if (ret == OSAL_SUCCESS) {
        ACL_InvalidateAffectedTelemetry(cmd_type);
    }

    return ret;
}

/**
 * @brief 处理遥测请求（从缓存读取）
 */
static int32_t handle_telemetry_request(pmc_tm_function_t tm_id, telemetry_response_t *response)
{
    /* 从缓存读取（<50μs） */
    int32_t ret = ACL_TelemetryCache_Read(tm_id, response);
    if (ret != OSAL_SUCCESS) {
        LOG_WARN("TC", "遥测读取失败: %d", tm_id);
        return ret;
    }

    /* 检查新鲜度 */
    if (response->freshness == TM_STATUS_STALE) {
        LOG_DEBUG("TC", "遥测数据过期: %d, age=%ums", tm_id, response->age_ms);
    }

    return OSAL_SUCCESS;
}

/**
 * @brief CAN接收任务
 */
static void* can_rx_task(void *arg)
{
    (void)arg;

    LOG_INFO("TC", "CAN接收任务启动");

    uint32_t cycle_count = 0;

    while (g_running) {
        cycle_count++;

        /* TODO: 接收CAN消息 */
        /* 模拟处理遥控命令 */
        if (cycle_count % 100 == 0) {
            pmc_tc_function_t cmd = TC_SERVER_POWER_ON;
            handle_telecommand(cmd, 0);
        }

        /* TODO: 处理遥测请求 */
        if (cycle_count % 50 == 0) {
            telemetry_response_t response;
            pmc_tm_function_t tm_id = TM_SERVER_CPU_TEMP;
            handle_telemetry_request(tm_id, &response);
        }

        /* 10ms周期 */
        OSAL_msleep(10);
    }

    LOG_INFO("TC", "CAN接收任务退出");
    return NULL;
}

/**
 * @brief 主函数
 */
int32_t main(int32_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    LOG_INFO("TC", "========================================");
    LOG_INFO("TC", "  Telecommand Process (Real-time)");
    LOG_INFO("TC", "========================================");

    /* 注册信号处理 */
    OSAL_SignalRegister(SIGINT, signal_handler);
    OSAL_SignalRegister(SIGTERM, signal_handler);

    /* 设置实时调度 */
    if (setup_realtime_scheduling() != OSAL_SUCCESS) {
        LOG_ERROR("TC", "实时调度配置失败");
        return -1;
    }

    /* 初始化ACL */
    if (ACL_Init() != OSAL_SUCCESS) {
        LOG_ERROR("TC", "ACL初始化失败");
        return -1;
    }

    /* 初始化遥测缓存 */
    if (ACL_TelemetryCache_Init() != OSAL_SUCCESS) {
        LOG_ERROR("TC", "遥测缓存初始化失败");
        return -1;
    }

    LOG_INFO("TC", "初始化完成");

    /* 创建CAN接收任务 */
    osal_thread_t can_thread;
    if (OSAL_ThreadCreate(&can_thread, can_rx_task, NULL) != OSAL_SUCCESS) {
        LOG_ERROR("TC", "创建CAN接收任务失败");
        return -1;
    }

    /* 主循环 */
    while (g_running) {
        OSAL_sleep(1);

        /* 打印统计信息 */
        uint32_t total, valid, fresh, stale;
        ACL_TelemetryCache_GetStats(&total, &valid, &fresh, &stale);
        LOG_INFO("TC", "缓存统计: total=%u, valid=%u, fresh=%u, stale=%u",
                 total, valid, fresh, stale);
    }

    /* 等待线程退出 */
    OSAL_ThreadJoin(can_thread);

    /* 清理 */
    ACL_TelemetryCache_Deinit();

    LOG_INFO("TC", "进程退出");

    return 0;
}
