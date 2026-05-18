/**
 * @file acl_tc.h
 * @brief ACL遥控功能枚举定义（通用）
 */

#ifndef ACL_TC_H
#define ACL_TC_H

#include "osal_types.h"

/**
 * @brief 遥控功能通用枚举
 * @note 按功能类别分组，预留足够ID空间支持扩展
 */
typedef enum {
    /* 电源控制类 (0-99) */
    TC_POWER_ON = 0,
    TC_POWER_OFF = 1,
    TC_POWER_RESET = 2,
    TC_POWER_CYCLE = 3,

    /* 复位控制类 (100-199) */
    TC_SOFT_RESET = 100,
    TC_HARD_RESET = 101,
    TC_MCU_RESET = 102,
    TC_MCU_POWER_CTRL = 103,
    TC_FPGA_RESET = 104,
    TC_FPGA_CONFIG_LOAD = 105,

    /* 固件升级类 (200-299) */
    TC_FIRMWARE_UPGRADE_START = 200,
    TC_FIRMWARE_UPGRADE_DATA = 201,
    TC_FIRMWARE_UPGRADE_VERIFY = 202,
    TC_FIRMWARE_UPGRADE_COMMIT = 203,

    /* 系统控制类 (300-399) */
    TC_SYSTEM_RESET = 300,
    TC_WATCHDOG_ENABLE = 301,
    TC_WATCHDOG_DISABLE = 302,

    /* 预留扩展空间 */
    TC_FUNC_MAX = 1000
} acl_tc_function_t;

#endif /* ACL_TC_H */
