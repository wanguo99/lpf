/************************************************************************
 * PCONFIG 重构后的使用示例
 *
 * 说明：
 * - 展示如何使用新的索引访问方式
 * - 展示 ACONFIG 设备映射的使用
 ************************************************************************/

#include "osal.h"
#include "pconfig.h"
#include "h200_device_map.h"

/*===========================================================================
 * 示例1：通过 ACONFIG 设备映射访问硬件
 *===========================================================================*/

void example_use_device_map(void)
{
    const pconfig_platform_config_t *platform;
    const h200_tc_device_map_t *tc_map;
    const pconfig_mcu_entry_t *payload_mcu;
    const pconfig_mcu_entry_t *power_mcu;
    const pconfig_bmc_entry_t *main_bmc;

    /* 1. 获取平台配置 */
    platform = PCONFIG_GetBoard();
    if (!platform) {
        LOG_ERROR("APP", "Failed to get platform config");
        return;
    }

    /* 2. 获取业务设备映射 */
    tc_map = H200_GetTCDeviceMap();

    /* 3. 通过映射索引访问具体硬件 */
    payload_mcu = PCONFIG_HW_GetMCU(platform, tc_map->payload_mcu_index);
    power_mcu = PCONFIG_HW_GetMCU(platform, tc_map->power_mcu_index);
    main_bmc = PCONFIG_HW_GetBMC(platform, tc_map->main_bmc_index);

    /* 4. 使用硬件配置 */
    if (payload_mcu && payload_mcu->enabled) {
        LOG_INFO("APP", "Init payload MCU: %s", payload_mcu->description);
        /* PDL_MCU_init(&payload_mcu->config); */
    }

    if (power_mcu && power_mcu->enabled) {
        LOG_INFO("APP", "Init power MCU: %s", power_mcu->description);
        /* PDL_MCU_init(&power_mcu->config); */
    }

    if (main_bmc && main_bmc->enabled) {
        LOG_INFO("APP", "Init main BMC: %s", main_bmc->description);
        /* PDL_BMC_init(&main_bmc->config); */
    }
}

/*===========================================================================
 * 示例2：健康监控 - 遍历所有需要监控的设备
 *===========================================================================*/

void example_health_monitor(void)
{
    const pconfig_platform_config_t *platform;
    const h200_health_device_map_t *health_map;
    uint32_t i;

    platform = PCONFIG_GetBoard();
    health_map = H200_GetHealthDeviceMap();

    LOG_INFO("HEALTH", "Starting health monitoring...");

    /* 监控所有 MCU */
    for (i = 0; i < health_map->mcu_count; i++) {
        uint32_t mcu_idx = health_map->mcu_indices[i];
        const pconfig_mcu_entry_t *mcu = PCONFIG_HW_GetMCU(platform, mcu_idx);

        if (mcu && mcu->enabled) {
            LOG_INFO("HEALTH", "Monitor MCU[%u]: %s", mcu_idx, mcu->description);
            /* 执行健康检查 */
        }
    }

    /* 监控所有 BMC */
    for (i = 0; i < health_map->bmc_count; i++) {
        uint32_t bmc_idx = health_map->bmc_indices[i];
        const pconfig_bmc_entry_t *bmc = PCONFIG_HW_GetBMC(platform, bmc_idx);

        if (bmc && bmc->enabled) {
            LOG_INFO("HEALTH", "Monitor BMC[%u]: %s", bmc_idx, bmc->description);
            /* 执行健康检查 */
        }
    }
}

/*===========================================================================
 * 示例3：直接索引访问（不使用映射）
 *===========================================================================*/

void example_direct_index_access(void)
{
    const pconfig_platform_config_t *platform;
    const pconfig_mcu_entry_t *mcu;
    uint32_t i;

    platform = PCONFIG_GetBoard();

    /* 遍历所有 MCU */
    LOG_INFO("APP", "Platform has %u MCUs:", platform->mcu_count);
    for (i = 0; i < platform->mcu_count; i++) {
        mcu = PCONFIG_HW_GetMCU(platform, i);
        if (mcu) {
            LOG_INFO("APP", "  MCU[%u]: %s (enabled=%d)",
                     i, mcu->description, mcu->enabled);
        }
    }
}

/*===========================================================================
 * 示例4：动态设备分配
 *===========================================================================*/

void example_dynamic_allocation(void)
{
    const pconfig_platform_config_t *platform;
    uint32_t primary_mcu_idx;
    uint32_t backup_mcu_idx;
    const pconfig_mcu_entry_t *mcu;

    platform = PCONFIG_GetBoard();

    /* 根据运行时条件选择设备 */
    if (/* 正常模式 */ 1) {
        primary_mcu_idx = 0;
        backup_mcu_idx = 1;
    } else {
        /* 冗余模式：交换主备 */
        primary_mcu_idx = 1;
        backup_mcu_idx = 0;
    }

    /* 使用选定的设备 */
    mcu = PCONFIG_HW_GetMCU(platform, primary_mcu_idx);
    if (mcu && mcu->enabled) {
        LOG_INFO("APP", "Using primary MCU[%u]: %s",
                 primary_mcu_idx, mcu->description);
    } else {
        /* 故障切换到备用设备 */
        mcu = PCONFIG_HW_GetMCU(platform, backup_mcu_idx);
        LOG_INFO("APP", "Failover to backup MCU[%u]: %s",
                 backup_mcu_idx, mcu->description);
    }
}

/*===========================================================================
 * 主函数
 *===========================================================================*/

/* External platform configuration */
extern const pconfig_platform_config_t pconfig_h200_100p_am625;

int main(void)
{
    /* 初始化 */
    PCONFIG_init();
    PCONFIG_register(&pconfig_h200_100p_am625);

    LOG_INFO("EXAMPLE", "=== PCONFIG Usage Examples ===");

    LOG_INFO("EXAMPLE", "\n--- Example 1: Device Mapping ---");
    example_use_device_map();

    LOG_INFO("EXAMPLE", "\n--- Example 2: Health Monitor ---");
    example_health_monitor();

    LOG_INFO("EXAMPLE", "\n--- Example 3: Direct Index Access ---");
    example_direct_index_access();

    LOG_INFO("EXAMPLE", "\n--- Example 4: Dynamic Allocation ---");
    example_dynamic_allocation();

    /* 清理 */
    PCONFIG_cleanup();

    return 0;
}
