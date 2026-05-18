/**
 * @file acl_demo_v1.c
 * @brief Demo应用配置 - 用于演示和测试
 * @note 命名规范：acl_{project}_{product}_{version}.c
 *       ACL配置关注业务功能，不关注硬件平台
 */

#include "acl_config.h"
#include "pmc_acl_types.h"

/**
 * @brief Demo遥控配置表
 * @note 简化配置，仅包含基本功能用于演示
 */
const acl_tc_config_t g_pmc_tc_configs[] = {
    /* 服务器电源控制 → BMC */
    { TC_SERVER_POWER_ON,      ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_POWER_OFF,     ACL_DEVICE_BMC, 0, true },
    { TC_SERVER_POWER_RESET,   ACL_DEVICE_BMC, 0, true },

    /* MCU控制 → MCU[0] */
    { TC_MCU_RESET,            ACL_DEVICE_MCU, 0, true },

    /* 系统控制 */
    { TC_SYSTEM_RESET,         ACL_DEVICE_MCU, 0, true },
    { TC_WATCHDOG_ENABLE,      ACL_DEVICE_MCU, 0, true },
    { TC_WATCHDOG_DISABLE,     ACL_DEVICE_MCU, 0, true },
};

const uint32_t g_pmc_tc_config_count = sizeof(g_pmc_tc_configs) / sizeof(acl_tc_config_t);

/**
 * @brief Demo遥测配置表
 * @note 简化配置，仅包含基本遥测用于演示
 *       所有遥测都从缓存读取，后台进程负责更新
 *       validity_ms：有效期，超过此时间标记为STALE
 *       update_period_ms：后台采集周期
 */
const acl_tm_config_t g_pmc_tm_configs[] = {
    /* 服务器基本遥测 → BMC，1秒更新周期，2秒有效期 */
    { TM_SERVER_CPU_TEMP,      ACL_DEVICE_BMC, 0, 2000, 1000, true },
    { TM_SERVER_BOARD_TEMP,    ACL_DEVICE_BMC, 0, 2000, 1000, true },
    { TM_SERVER_POWER_STATUS,  ACL_DEVICE_BMC, 0, 500,  100,  true },

    /* MCU基本遥测 → MCU，2秒更新周期，4秒有效期 */
    { TM_MCU_STATUS,           ACL_DEVICE_MCU, 0, 500,  100,  true },
    { TM_MCU_TEMP,             ACL_DEVICE_MCU, 0, 4000, 2000, true },

    /* 系统遥测 */
    { TM_SYSTEM_UPTIME,        ACL_DEVICE_MCU, 0, 4000, 2000, true },
    { TM_WATCHDOG_STATUS,      ACL_DEVICE_MCU, 0, 500,  100,  true },
};

const uint32_t g_pmc_tm_config_count = sizeof(g_pmc_tm_configs) / sizeof(acl_tm_config_t);
