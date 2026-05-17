/**
 * @file acl_demo_v1_invalidation.c
 * @brief Demo遥控命令失效映射 - 用于演示和测试
 * @note 定义遥控命令对遥测数据的影响关系
 */

#include "acl_config.h"
#include "pmc_acl_types.h"

/**
 * @brief Demo遥控命令失效映射表
 * @note 遥控命令执行后，相关遥测缓存标记为STALE，等待重新采集
 *       简化配置，仅包含基本映射关系用于演示
 */
const tc_tm_invalidation_map_t g_invalidation_map[] = {
    /* 电源控制命令 → 影响电源状态遥测 */
    {
        .tc_function = TC_SERVER_POWER_ON,
        .affected_tm = { TM_SERVER_POWER_STATUS },
        .affected_count = 1
    },
    {
        .tc_function = TC_SERVER_POWER_OFF,
        .affected_tm = { TM_SERVER_POWER_STATUS },
        .affected_count = 1
    },
    {
        .tc_function = TC_SERVER_POWER_RESET,
        .affected_tm = { TM_SERVER_POWER_STATUS, TM_SERVER_CPU_TEMP },
        .affected_count = 2
    },

    /* MCU复位 → 影响MCU状态遥测 */
    {
        .tc_function = TC_MCU_RESET,
        .affected_tm = { TM_MCU_STATUS, TM_MCU_TEMP },
        .affected_count = 2
    },

    /* 看门狗控制 → 影响看门狗状态遥测 */
    {
        .tc_function = TC_WATCHDOG_ENABLE,
        .affected_tm = { TM_WATCHDOG_STATUS },
        .affected_count = 1
    },
    {
        .tc_function = TC_WATCHDOG_DISABLE,
        .affected_tm = { TM_WATCHDOG_STATUS },
        .affected_count = 1
    },

    /* 系统复位 → 影响系统相关遥测 */
    {
        .tc_function = TC_SYSTEM_RESET,
        .affected_tm = {
            TM_MCU_STATUS, TM_MCU_TEMP,
            TM_SYSTEM_UPTIME
        },
        .affected_count = 3
    },
};

const uint32_t g_invalidation_map_count = sizeof(g_invalidation_map) / sizeof(tc_tm_invalidation_map_t);
