/**
 * @file acl_tm.h
 * @brief ACL遥测功能枚举定义（通用）
 */

#ifndef ACL_TM_H
#define ACL_TM_H

#include "osal_types.h"

/**
 * @brief 遥测功能通用枚举
 * @note 按功能类别分组，预留足够ID空间支持扩展
 */
typedef enum {
    /* 温度遥测类 (0-99) */
    TM_CPU_TEMP = 0,
    TM_BOARD_TEMP = 1,
    TM_MCU_TEMP = 2,
    TM_FPGA_TEMP = 3,
    TM_FAN_SPEED = 4,

    /* 电压遥测类 (100-199) */
    TM_VOLTAGE_12V = 100,
    TM_VOLTAGE_5V = 101,
    TM_VOLTAGE_3V3 = 102,
    TM_CURRENT = 103,

    /* 状态遥测类 (200-299) */
    TM_POWER_STATUS = 200,
    TM_MCU_STATUS = 201,
    TM_FPGA_STATUS = 202,
    TM_FPGA_CONFIG_STATUS = 203,
    TM_WATCHDOG_STATUS = 204,

    /* 系统遥测类 (300-399) */
    TM_SYSTEM_UPTIME = 300,
    TM_MCU_UPTIME = 301,
    TM_ERROR_COUNT = 302,

    /* 预留扩展空间 */
    TM_FUNC_MAX = 1000
} acl_tm_function_t;

#endif /* ACL_TM_H */
