#include "pmc_collector.h"
#include "libpmc_ipc.h"
#include "libpmc_protocol.h"
#include <signal.h>

/* 全局变量 */
static pmc_tm_cache_t *g_tm_cache = NULL;
static pmc_process_heartbeat_t *g_heartbeat = NULL;
static pmc_system_status_t *g_status = NULL;
static volatile bool g_running = true;

/* 信号处理 */
static void signal_handler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        g_running = false;
        LOG_INFO("COLLECTOR", "收到退出信号");
    }
}

/* 初始化 */
int32_t PMC_Collector_Init(void)
{
    int32_t ret;

    LOG_INFO("COLLECTOR", "Collector进程初始化...");

    /* 注册信号处理 */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    /* 初始化遥测缓存 */
    ret = PMC_TM_Cache_Init(&g_tm_cache);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("COLLECTOR", "初始化遥测缓存失败: %d", ret);
        return ret;
    }

    /* 初始化心跳 */
    ret = PMC_Heartbeat_Init(&g_heartbeat);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("COLLECTOR", "初始化心跳失败: %d", ret);
        PMC_TM_Cache_Cleanup(g_tm_cache);
        return ret;
    }

    /* 初始化系统状态 */
    ret = PMC_Status_Init(&g_status);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("COLLECTOR", "初始化系统状态失败: %d", ret);
        PMC_TM_Cache_Cleanup(g_tm_cache);
        PMC_Heartbeat_Cleanup(g_heartbeat);
        return ret;
    }

    /* 绑定到CPU1（建议） */
    ret = OSAL_SchedSetAffinity(OSAL_ThreadSelf(), 1);
    if (ret != OSAL_SUCCESS) {
        LOG_WARN("COLLECTOR", "绑定CPU1失败: %d", ret);
    }

    LOG_INFO("COLLECTOR", "Collector进程初始化完成");
    return OSAL_SUCCESS;
}

/* 采集服务器遥测（BMC） */
static int32_t collect_server_telemetry(void)
{
    /* TODO: 通过PDL_BMC接口采集服务器遥测
     * - CPU温度
     * - 板卡温度
     * - 服务器状态
     */

    /* 模拟数据 */
    uint8_t data[4];
    int32_t cpu_temp = 45;  /* 45℃ */

    data[0] = (cpu_temp >> 24) & 0xFF;
    data[1] = (cpu_temp >> 16) & 0xFF;
    data[2] = (cpu_temp >> 8) & 0xFF;
    data[3] = cpu_temp & 0xFF;

    /* 写入缓存，有效期2秒 */
    PMC_TM_Cache_Write(g_tm_cache, PMC_TM_CPU_TEMP, data, 4, 2000);

    return OSAL_SUCCESS;
}

/* 采集电源遥测（MCU） */
static int32_t collect_power_telemetry(void)
{
    /* TODO: 通过PDL_MCU接口采集电源遥测
     * - 54V电压
     * - 12V电压
     * - MCU状态
     */

    /* 模拟数据 */
    uint8_t data[4];
    uint32_t voltage_54v = 54000;  /* 54V = 54000mV */

    data[0] = (voltage_54v >> 24) & 0xFF;
    data[1] = (voltage_54v >> 16) & 0xFF;
    data[2] = (voltage_54v >> 8) & 0xFF;
    data[3] = voltage_54v & 0xFF;

    /* 写入缓存，有效期500ms */
    PMC_TM_Cache_Write(g_tm_cache, PMC_TM_VOLTAGE_54V, data, 4, 500);

    return OSAL_SUCCESS;
}

/* 采集温度遥测（I2C） */
static int32_t collect_temperature_telemetry(void)
{
    /* TODO: 通过HAL_I2C接口采集温度传感器数据
     * - PCB温度
     */

    /* 模拟数据 */
    uint8_t data[4];
    int32_t board_temp = 40;  /* 40℃ */

    data[0] = (board_temp >> 24) & 0xFF;
    data[1] = (board_temp >> 16) & 0xFF;
    data[2] = (board_temp >> 8) & 0xFF;
    data[3] = board_temp & 0xFF;

    /* 写入缓存，有效期4秒 */
    PMC_TM_Cache_Write(g_tm_cache, PMC_TM_BOARD_TEMP, data, 4, 4000);

    return OSAL_SUCCESS;
}

/* 更新系统状态 */
static int32_t update_system_status(void)
{
    pmc_system_status_t status;

    /* TODO: 从各个外设获取状态 */
    status.server_online = true;
    status.mcu_online = true;
    status.fpga_online = true;
    status.cpld_online = true;
    status.cpu_temp = 45;
    status.board_temp = 40;
    status.voltage_54v = 54000;
    status.voltage_12v = 12000;

    /* 写入共享内存 */
    PMC_Status_Write(g_status, &status);

    return OSAL_SUCCESS;
}

/* 主循环 */
int32_t PMC_Collector_Run(void)
{
    uint32_t fast_cycle_count = 0;  /* 快遥周期计数 */
    uint32_t slow_cycle_count = 0;  /* 慢遥周期计数 */

    LOG_INFO("COLLECTOR", "Collector进程开始运行");

    while (g_running) {
        /* 更新心跳 */
        PMC_Heartbeat_Update(g_heartbeat, PMC_PROCESS_COLLECTOR);

        /* 快遥采集（100ms周期） */
        if (fast_cycle_count % 1 == 0) {
            collect_server_telemetry();
            collect_power_telemetry();
        }

        /* 慢遥采集（2s周期 = 20 * 100ms） */
        if (slow_cycle_count % 20 == 0) {
            collect_temperature_telemetry();
        }

        /* 更新系统状态 */
        update_system_status();

        /* 休眠100ms */
        OSAL_msleep(100);

        fast_cycle_count++;
        slow_cycle_count++;
    }

    LOG_INFO("COLLECTOR", "Collector进程退出");
    return OSAL_SUCCESS;
}

/* 清理 */
void PMC_Collector_Cleanup(void)
{
    LOG_INFO("COLLECTOR", "Collector进程清理...");

    if (g_tm_cache) {
        PMC_TM_Cache_Cleanup(g_tm_cache);
        g_tm_cache = NULL;
    }

    if (g_heartbeat) {
        PMC_Heartbeat_Cleanup(g_heartbeat);
        g_heartbeat = NULL;
    }

    if (g_status) {
        PMC_Status_Cleanup(g_status);
        g_status = NULL;
    }

    LOG_INFO("COLLECTOR", "Collector进程清理完成");
}
