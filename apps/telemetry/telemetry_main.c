/**
 * @file telemetry_main.c
 * @brief Telemetry进程主程序
 * @note 后台进程，CPU1，优先级0
 *       职责：
 *       1. 周期性采集BMC/MCU/FPGA数据
 *       2. 更新共享内存缓存
 *       3. 更新时间戳和新鲜度标记
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
 * @brief 设置CPU亲和性
 */
static int32_t setup_cpu_affinity(void)
{
    /* 绑定到CPU1 */
    int32_t ret = OSAL_SchedSetAffinity(0, 1);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR("TM", "设置CPU亲和性失败: %d", ret);
        return ret;
    }

    LOG_INFO("TM", "CPU亲和性配置完成: CPU=1");

    return OSAL_SUCCESS;
}

/**
 * @brief 采集单个遥测数据
 */
static int32_t collect_telemetry(pmc_tm_function_t tm_id)
{
    /* 通过ACL查询设备映射 */
    const acl_tm_config_t *cfg = ACL_GetTmConfig(tm_id);
    if (!cfg || !cfg->enabled) {
        return OSAL_ERR_GENERIC;
    }

    /* 模拟采集数据 */
    uint8_t data[256];
    uint32_t data_len = 0;

    switch (cfg->device_type) {
        case ACL_DEVICE_BMC:
            /* TODO: 调用BMC PDL接口采集数据 */
            /* 模拟数据 */
            data[0] = 45;  /* 温度45度 */
            data_len = 1;
            LOG_DEBUG("TM", "采集BMC遥测: %d", tm_id);
            break;

        case ACL_DEVICE_MCU:
            /* TODO: 调用MCU PDL接口采集数据 */
            /* 模拟数据 */
            data[0] = 1;  /* 状态正常 */
            data_len = 1;
            LOG_DEBUG("TM", "采集MCU遥测: %d", tm_id);
            break;

        case ACL_DEVICE_FPGA:
            /* TODO: 调用FPGA PDL接口采集数据 */
            /* 模拟数据 */
            data[0] = 1;  /* 状态正常 */
            data_len = 1;
            LOG_DEBUG("TM", "采集FPGA遥测: %d", tm_id);
            break;

        default:
            return OSAL_ERR_NOT_IMPLEMENTED;
    }

    /* 写入缓存 */
    if (data_len > 0) {
        return ACL_TelemetryCache_Write(tm_id, data, data_len);
    }

    return OSAL_ERR_GENERIC;
}

/**
 * @brief 采集任务
 */
static void* collection_task(void *arg)
{
    (void)arg;

    LOG_INFO("TM", "采集任务启动");

    uint32_t cycle_count = 0;

    while (g_running) {
        cycle_count++;
        LOG_DEBUG("TM", "采集周期: %u", cycle_count);

        /* 遍历所有遥测配置 */
        for (uint32_t i = 0; i < TM_FUNC_MAX; i++) {
            const acl_tm_config_t *cfg = ACL_GetTmConfig(i);
            if (!cfg || !cfg->enabled) {
                continue;
            }

            /* 根据更新周期判断是否需要采集 */
            /* 这里简化处理，每个周期都采集 */
            collect_telemetry(i);
        }

        /* 100ms周期 */
        OSAL_msleep(100);
    }

    LOG_INFO("TM", "采集任务退出");
    return NULL;
}

/**
 * @brief 主函数
 */
int32_t main(int32_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    LOG_INFO("TM", "========================================");
    LOG_INFO("TM", "  Telemetry Process (Background)");
    LOG_INFO("TM", "========================================");

    /* 注册信号处理 */
    OSAL_SignalRegister(SIGINT, signal_handler);
    OSAL_SignalRegister(SIGTERM, signal_handler);

    /* 设置CPU亲和性 */
    if (setup_cpu_affinity() != OSAL_SUCCESS) {
        LOG_ERROR("TM", "CPU亲和性配置失败");
        return -1;
    }

    /* 初始化ACL */
    if (ACL_Init() != OSAL_SUCCESS) {
        LOG_ERROR("TM", "ACL初始化失败");
        return -1;
    }

    /* 初始化遥测缓存 */
    if (ACL_TelemetryCache_Init() != OSAL_SUCCESS) {
        LOG_ERROR("TM", "遥测缓存初始化失败");
        return -1;
    }

    LOG_INFO("TM", "初始化完成");

    /* 创建采集任务 */
    osal_thread_t collection_thread;
    if (OSAL_ThreadCreate(&collection_thread, collection_task, NULL) != OSAL_SUCCESS) {
        LOG_ERROR("TM", "创建采集任务失败");
        return -1;
    }

    /* 主循环 */
    while (g_running) {
        OSAL_sleep(5);

        /* 打印统计信息 */
        uint32_t total, valid, fresh, stale;
        ACL_TelemetryCache_GetStats(&total, &valid, &fresh, &stale);
        LOG_INFO("TM", "缓存统计: total=%u, valid=%u, fresh=%u, stale=%u",
                 total, valid, fresh, stale);
    }

    /* 等待线程退出 */
    OSAL_ThreadJoin(collection_thread);

    /* 清理 */
    ACL_TelemetryCache_Deinit();

    LOG_INFO("TM", "进程退出");

    return 0;
}
