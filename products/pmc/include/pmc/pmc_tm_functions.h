/**
 * @file pmc_tm_functions.h
 * @brief PMC 遥测功能定义
 * @note 产品层：定义 PMC 项目的遥测功能枚举
 *       从 core/aconfig/aconfig_tm.h 移动到产品层
 */

#ifndef PMC_TM_FUNCTIONS_H
#define PMC_TM_FUNCTIONS_H

#include <stdint.h>

/**
 * @brief PMC 遥测功能枚举
 * @note 按功能类别分组，预留足够 ID 空间支持扩展
 */
typedef enum {
    /* 温度遥测类 (0-99) */
    PMC_TM_CPU_TEMP = 0x0,
    PMC_TM_BOARD_TEMP = 0x1,
    PMC_TM_MCU_TEMP = 0x2,
    PMC_TM_FPGA_TEMP = 0x3,
    PMC_TM_FAN_SPEED = 0x4,

    /* 电压遥测类 (100-199) */
    PMC_TM_VOLTAGE_12V = 0x64,
    PMC_TM_VOLTAGE_5V = 0x65,
    PMC_TM_VOLTAGE_3V3 = 0x66,
    PMC_TM_CURRENT = 0x67,

    /* 状态遥测类 (200-299) */
    PMC_TM_POWER_STATUS = 0xC8,
    PMC_TM_MCU_STATUS = 0xC9,
    PMC_TM_FPGA_STATUS = 0xCA,
    PMC_TM_FPGA_CONFIG_STATUS = 0xCB,

    /* 系统遥测类 (300-399) */
    PMC_TM_SYSTEM_UPTIME = 0x12C,
    PMC_TM_MCU_UPTIME = 0x12D,
    PMC_TM_ERROR_COUNT = 0x12E,

    /* 预留扩展空间 */
    PMC_TM_FUNC_MAX = 0x3E8
} pmc_tm_function_t;

/* 兼容性别名（逐步迁移） */
#define ACONFIG_TM_CPU_TEMP           PMC_TM_CPU_TEMP
#define ACONFIG_TM_BOARD_TEMP         PMC_TM_BOARD_TEMP
#define ACONFIG_TM_MCU_TEMP           PMC_TM_MCU_TEMP
#define ACONFIG_TM_FPGA_TEMP          PMC_TM_FPGA_TEMP
#define ACONFIG_TM_FAN_SPEED          PMC_TM_FAN_SPEED
#define ACONFIG_TM_VOLTAGE_12V        PMC_TM_VOLTAGE_12V
#define ACONFIG_TM_VOLTAGE_5V         PMC_TM_VOLTAGE_5V
#define ACONFIG_TM_VOLTAGE_3V3        PMC_TM_VOLTAGE_3V3
#define ACONFIG_TM_CURRENT            PMC_TM_CURRENT
#define ACONFIG_TM_POWER_STATUS       PMC_TM_POWER_STATUS
#define ACONFIG_TM_MCU_STATUS         PMC_TM_MCU_STATUS
#define ACONFIG_TM_FPGA_STATUS        PMC_TM_FPGA_STATUS
#define ACONFIG_TM_FPGA_CONFIG_STATUS PMC_TM_FPGA_CONFIG_STATUS
#define ACONFIG_TM_SYSTEM_UPTIME      PMC_TM_SYSTEM_UPTIME
#define ACONFIG_TM_MCU_UPTIME         PMC_TM_MCU_UPTIME
#define ACONFIG_TM_ERROR_COUNT        PMC_TM_ERROR_COUNT
#define ACONFIG_TM_FUNC_MAX           PMC_TM_FUNC_MAX

#endif /* PMC_TM_FUNCTIONS_H */
