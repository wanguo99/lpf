/**
 * @file pmc_tc_functions.h
 * @brief PMC 遥控功能定义
 * @note 产品层：定义 PMC 项目的遥控功能枚举
 *       从 core/aconfig/aconfig_tc.h 移动到产品层
 */

#ifndef PMC_TC_FUNCTIONS_H
#define PMC_TC_FUNCTIONS_H

#include <stdint.h>

/**
 * @brief PMC 遥控功能枚举
 * @note 按功能类别分组，预留足够 ID 空间支持扩展
 */
typedef enum {
    /* 电源控制类 (0-99) */
    PMC_TC_POWER_ON = 0x0,
    PMC_TC_POWER_OFF = 0x1,
    PMC_TC_POWER_RESET = 0x2,
    PMC_TC_POWER_CYCLE = 0x3,

    /* 复位控制类 (100-199) */
    PMC_TC_SOFT_RESET = 0x64,
    PMC_TC_HARD_RESET = 0x65,
    PMC_TC_MCU_RESET = 0x66,
    PMC_TC_MCU_POWER_CTRL = 0x67,
    PMC_TC_FPGA_RESET = 0x68,
    PMC_TC_FPGA_CONFIG_LOAD = 0x69,

    /* 固件升级类 (200-299) */
    PMC_TC_FIRMWARE_UPGRADE_START = 0xC8,
    PMC_TC_FIRMWARE_UPGRADE_DATA = 0xC9,
    PMC_TC_FIRMWARE_UPGRADE_VERIFY = 0xCA,
    PMC_TC_FIRMWARE_UPGRADE_COMMIT = 0xCB,

    /* 系统控制类 (300-399) */
    PMC_TC_SYSTEM_RESET = 0x12C,

    /* 预留扩展空间 */
    PMC_TC_FUNC_MAX = 0x3E8
} pmc_tc_function_t;

/* 兼容性别名（逐步迁移） */
#define ACONFIG_TC_POWER_ON                PMC_TC_POWER_ON
#define ACONFIG_TC_POWER_OFF               PMC_TC_POWER_OFF
#define ACONFIG_TC_POWER_RESET             PMC_TC_POWER_RESET
#define ACONFIG_TC_POWER_CYCLE             PMC_TC_POWER_CYCLE
#define ACONFIG_TC_SOFT_RESET              PMC_TC_SOFT_RESET
#define ACONFIG_TC_HARD_RESET              PMC_TC_HARD_RESET
#define ACONFIG_TC_MCU_RESET               PMC_TC_MCU_RESET
#define ACONFIG_TC_MCU_POWER_CTRL          PMC_TC_MCU_POWER_CTRL
#define ACONFIG_TC_FPGA_RESET              PMC_TC_FPGA_RESET
#define ACONFIG_TC_FPGA_CONFIG_LOAD        PMC_TC_FPGA_CONFIG_LOAD
#define ACONFIG_TC_FIRMWARE_UPGRADE_START  PMC_TC_FIRMWARE_UPGRADE_START
#define ACONFIG_TC_FIRMWARE_UPGRADE_DATA   PMC_TC_FIRMWARE_UPGRADE_DATA
#define ACONFIG_TC_FIRMWARE_UPGRADE_VERIFY PMC_TC_FIRMWARE_UPGRADE_VERIFY
#define ACONFIG_TC_FIRMWARE_UPGRADE_COMMIT PMC_TC_FIRMWARE_UPGRADE_COMMIT
#define ACONFIG_TC_SYSTEM_RESET            PMC_TC_SYSTEM_RESET
#define ACONFIG_TC_FUNC_MAX                PMC_TC_FUNC_MAX

#endif /* PMC_TC_FUNCTIONS_H */
