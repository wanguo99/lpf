/**
 * @file acl_h200_100p_invalidation.c
 * @brief H200_100P遥控命令失效映射
 * @note 定义遥控命令对遥测数据的影响关系
 */

#include "acl_config.h"
#include "pmc_acl_types.h"

/**
 * @brief PMC v1.0遥控命令失效映射表
 * @note 遥控命令执行后，相关遥测缓存标记为STALE，等待重新采集
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
    {
        .tc_function = TC_SERVER_POWER_CYCLE,
        .affected_tm = { TM_SERVER_POWER_STATUS, TM_SERVER_CPU_TEMP },
        .affected_count = 2
    },

    /* 服务器复位 → 影响服务器状态遥测 */
    {
        .tc_function = TC_SERVER_SOFT_RESET,
        .affected_tm = { TM_SERVER_POWER_STATUS, TM_SERVER_CPU_TEMP },
        .affected_count = 2
    },
    {
        .tc_function = TC_SERVER_HARD_RESET,
        .affected_tm = { TM_SERVER_POWER_STATUS, TM_SERVER_CPU_TEMP },
        .affected_count = 2
    },

    /* MCU复位 → 影响MCU状态遥测 */
    {
        .tc_function = TC_MCU_RESET,
        .affected_tm = { TM_MCU_STATUS, TM_MCU_TEMP, TM_MCU_VOLTAGE, TM_MCU_UPTIME },
        .affected_count = 4
    },

    /* MCU电源控制 → 影响电压遥测 */
    {
        .tc_function = TC_MCU_POWER_CTRL,
        .affected_tm = { TM_SERVER_VOLTAGE_12V, TM_SERVER_VOLTAGE_5V, TM_SERVER_VOLTAGE_3V3 },
        .affected_count = 3
    },

    /* FPGA复位 → 影响FPGA状态遥测 */
    {
        .tc_function = TC_FPGA_RESET,
        .affected_tm = { TM_FPGA_STATUS, TM_FPGA_CONFIG_STATUS },
        .affected_count = 2
    },

    /* FPGA配置加载 → 影响FPGA配置状态 */
    {
        .tc_function = TC_FPGA_CONFIG_LOAD,
        .affected_tm = { TM_FPGA_CONFIG_STATUS },
        .affected_count = 1
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

    /* 系统复位 → 影响所有遥测 */
    {
        .tc_function = TC_SYSTEM_RESET,
        .affected_tm = {
            TM_MCU_STATUS, TM_MCU_TEMP, TM_MCU_VOLTAGE, TM_MCU_UPTIME,
            TM_SYSTEM_UPTIME, TM_ERROR_COUNT
        },
        .affected_count = 6
    },
};

const uint32_t g_invalidation_map_count = sizeof(g_invalidation_map) / sizeof(tc_tm_invalidation_map_t);
