/**
 * @file aconfig_tc.h
 * @brief ACONFIG 遥控功能定义 - 标准遥控功能枚举
 * @note 本文件定义标准的遥控功能枚举，项目可以扩展或自定义
 */

#ifndef ACONFIG_ACONFIG_TC_H
#define ACONFIG_ACONFIG_TC_H

#include <osal/osal_types.h>

/**
 * @brief 遥控功能通用枚举
 * @note 按功能类别分组，预留足够 ID 空间支持扩展
 */
typedef enum {
	/* 电源控制类 (0-99) */
	ACONFIG_TC_POWER_ON = 0,
	ACONFIG_TC_POWER_OFF = 1,
	ACONFIG_TC_POWER_RESET = 2,
	ACONFIG_TC_POWER_CYCLE = 3,

	/* 复位控制类 (100-199) */
	ACONFIG_TC_SOFT_RESET = 100,
	ACONFIG_TC_HARD_RESET = 101,
	ACONFIG_TC_MCU_RESET = 102,
	ACONFIG_TC_MCU_POWER_CTRL = 103,
	ACONFIG_TC_FPGA_RESET = 104,
	ACONFIG_TC_FPGA_CONFIG_LOAD = 105,

	/* 固件升级类 (200-299) */
	ACONFIG_TC_FIRMWARE_UPGRADE_START = 200,
	ACONFIG_TC_FIRMWARE_UPGRADE_DATA = 201,
	ACONFIG_TC_FIRMWARE_UPGRADE_VERIFY = 202,
	ACONFIG_TC_FIRMWARE_UPGRADE_COMMIT = 203,

	/* 系统控制类 (300-399) */
	ACONFIG_TC_SYSTEM_RESET = 300,
	ACONFIG_TC_WATCHDOG_ENABLE = 301,
	ACONFIG_TC_WATCHDOG_DISABLE = 302,

	/* 预留扩展空间 */
	ACONFIG_TC_FUNC_MAX = 1000
} aconfig_tc_function_t;

#endif /* ACONFIG_ACONFIG_TC_H */
