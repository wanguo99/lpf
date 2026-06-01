#include "ccm_health.h"
#include "libccm/libccm_ipc.h"
#include "sys/osal_signal.h"
#include <unistd.h>

/* 全局变量 */
static ccm_process_heartbeat_t *g_heartbeat = NULL;
static ccm_system_status_t *g_status = NULL;
static volatile bool g_running = true;

/* 线程ID */
static osal_thread_t g_satellite_hb_thread = 0;
static osal_thread_t g_mcu_fpga_hb_thread = 0;
static osal_thread_t g_cpld_hb_thread = 0;
static osal_thread_t g_watchdog_thread = 0;

/* 信号处理 */
static void signal_handler(int32_t sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        const char msg[] = "HEALTH: 收到退出信号\n";
        g_running = false;
        (void)write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
}

/* 线程1: 卫星平台心跳+状态监测（1秒周期） */
static void *satellite_heartbeat_thread(void *arg)
{
    uint32_t sequence = 0;

    (void)arg;

    LOG_INFO("HEALTH", "卫星心跳线程启动");

    while (g_running) {
        ccm_system_status_t status;

        /* 更新进程心跳 */
        CCM_Heartbeat_Update(g_heartbeat, CCM_PROCESS_HEALTH);

        /* TODO: 发送心跳到卫星平台（CAN） */
        LOG_DEBUG("HEALTH", "发送卫星心跳: seq=%u", sequence);

        /* TODO: 监测外设状态 */
        if (CCM_Status_Read(g_status, &status) == OSAL_SUCCESS) {
            if (!status.server_online) {
                LOG_WARN("HEALTH", "服务器离线");
            }
            if (status.cpu_temp > 80) {
                LOG_WARN("HEALTH", "CPU温度过高: %d℃", status.cpu_temp);
            }
        }

        sequence++;
        OSAL_msleep(1000);  /* 1秒 */
    }

    LOG_INFO("HEALTH", "卫星心跳线程退出");
    return NULL;
}

/* 线程2: MCU+FPGA心跳管理（500ms周期） */
static void *mcu_fpga_heartbeat_thread(void *arg)
{
    (void)arg;

    LOG_INFO("HEALTH", "MCU/FPGA心跳线程启动");

    while (g_running) {
        /* TODO: 发送心跳到MCU（CAN） */
        LOG_DEBUG("HEALTH", "发送MCU心跳");

        /* TODO: 发送心跳到FPGA（GPIO） */
        LOG_DEBUG("HEALTH", "发送FPGA心跳");

        /* TODO: 检测MCU心跳超时 */
        /* TODO: 检测FPGA心跳超时 */

        OSAL_msleep(500);  /* 500ms */
    }

    LOG_INFO("HEALTH", "MCU/FPGA心跳线程退出");
    return NULL;
}

/* 线程3: CPLD心跳握手（100ms周期） */
static void *cpld_heartbeat_thread(void *arg)
{
    (void)arg;

    LOG_INFO("HEALTH", "CPLD心跳线程启动");

    while (g_running) {
        /* TODO: GPIO翻转心跳信号 */
        LOG_DEBUG("HEALTH", "CPLD心跳握手");

        OSAL_msleep(100);  /* 100ms */
    }

    LOG_INFO("HEALTH", "CPLD心跳线程退出");
    return NULL;
}

/* 线程4: 硬件看门狗喂狗（4秒周期） */
static void *watchdog_thread(void *arg)
{
    (void)arg;

    LOG_INFO("HEALTH", "看门狗线程启动");

    while (g_running) {
        /* TODO: 检查系统健康状态 */
        bool system_healthy = true;

        if (system_healthy) {
            /* TODO: 喂狗 */
            LOG_DEBUG("HEALTH", "喂狗");
        } else {
            LOG_ERROR("HEALTH", "系统异常，停止喂狗");
            /* 停止喂狗，触发系统复位 */
        }

        OSAL_msleep(4000);  /* 4秒 */
    }

    LOG_INFO("HEALTH", "看门狗线程退出");
    return NULL;
}

/* 初始化 */
int32_t CCM_Health_Init(void)
{
    int32_t ret;

    LOG_INFO("HEALTH", "Health进程初始化...");

    /* 注册信号处理 */
    OSAL_SignalRegister(SIGTERM, signal_handler);
    OSAL_SignalRegister(SIGINT, signal_handler);

    /* 初始化心跳 */
    ret = CCM_Heartbeat_Init(&g_heartbeat);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("HEALTH", "初始化心跳失败: %d", ret);
        return ret;
    }

    /* 初始化系统状态 */
    ret = CCM_Status_Init(&g_status);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("HEALTH", "初始化系统状态失败: %d", ret);
        CCM_Heartbeat_Cleanup(g_heartbeat);
        return ret;
    }

    LOG_INFO("HEALTH", "Health进程初始化完成");
    return OSAL_SUCCESS;
}

/* 主循环 */
int32_t CCM_Health_Run(void)
{
    int32_t ret;

    LOG_INFO("HEALTH", "Health进程开始运行");

    /* 创建线程1: 卫星心跳 */
    ret = OSAL_ThreadCreate(&g_satellite_hb_thread, satellite_heartbeat_thread, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("HEALTH", "创建卫星心跳线程失败: %d", ret);
        return ret;
    }

    /* 创建线程2: MCU/FPGA心跳 */
    ret = OSAL_ThreadCreate(&g_mcu_fpga_hb_thread, mcu_fpga_heartbeat_thread, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("HEALTH", "创建MCU/FPGA心跳线程失败: %d", ret);
        return ret;
    }

    /* 创建线程3: CPLD心跳 */
    ret = OSAL_ThreadCreate(&g_cpld_hb_thread, cpld_heartbeat_thread, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("HEALTH", "创建CPLD心跳线程失败: %d", ret);
        return ret;
    }

    /* 创建线程4: 看门狗 */
    ret = OSAL_ThreadCreate(&g_watchdog_thread, watchdog_thread, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("HEALTH", "创建看门狗线程失败: %d", ret);
        return ret;
    }

    /* 等待退出信号 */
    while (g_running) {
        OSAL_msleep(1000);
    }

    /* 等待所有线程退出 */
    LOG_INFO("HEALTH", "等待线程退出...");
    OSAL_msleep(2000);

    LOG_INFO("HEALTH", "Health进程退出");
    return OSAL_SUCCESS;
}

/* 清理 */
void CCM_Health_Cleanup(void)
{
    LOG_INFO("HEALTH", "Health进程清理...");

    if (g_heartbeat) {
        CCM_Heartbeat_Cleanup(g_heartbeat);
        g_heartbeat = NULL;
    }

    if (g_status) {
        CCM_Status_Cleanup(g_status);
        g_status = NULL;
    }

    LOG_INFO("HEALTH", "Health进程清理完成");
}
