/**
 * @file aconfig_tm.h
 * @brief ACONFIG 遥测功能定义 - 标准遥测功能枚举
 * @note 本文件定义标准的遥测功能枚举，项目可以扩展或自定义
 */

#ifndef ACONFIG_ACONFIG_TM_H
#define ACONFIG_ACONFIG_TM_H

#include <osal/osal_types.h>

/**
 * @brief 遥测功能通用枚举
 * @note 按功能类别分组，预留足够 ID 空间支持扩展
 */
typedef enum {
	/* 温度遥测类 (0-99) */
	ACONFIG_TM_CPU_TEMP = 0,
	ACONFIG_TM_BOARD_TEMP = 1,
	ACONFIG_TM_MCU_TEMP = 2,
	ACONFIG_TM_FPGA_TEMP = 3,
	ACONFIG_TM_FAN_SPEED = 4,

	/* 电压遥测类 (100-199) */
	ACONFIG_TM_VOLTAGE_12V = 100,
	ACONFIG_TM_VOLTAGE_5V = 101,
	ACONFIG_TM_VOLTAGE_3V3 = 102,
	ACONFIG_TM_CURRENT = 103,

	/* 状态遥测类 (200-299) */
	ACONFIG_TM_POWER_STATUS = 200,
	ACONFIG_TM_MCU_STATUS = 201,
	ACONFIG_TM_FPGA_STATUS = 202,
	ACONFIG_TM_FPGA_CONFIG_STATUS = 203,
	ACONFIG_TM_WATCHDOG_STATUS = 204,

	/* 系统遥测类 (300-399) */
	ACONFIG_TM_SYSTEM_UPTIME = 300,
	ACONFIG_TM_MCU_UPTIME = 301,
	ACONFIG_TM_ERROR_COUNT = 302,

	/* 预留扩展空间 */
	ACONFIG_TM_FUNC_MAX = 1000
} aconfig_tm_function_t;

#endif /* ACONFIG_ACONFIG_TM_H */
