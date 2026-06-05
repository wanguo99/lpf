/**
 * @file aconfig_tm.h
 * @brief ACONFIG 遥测功能定义 - 标准遥测功能枚举
 * @note 本文件定义标准的遥测功能枚举，项目可以扩展或自定义
 */

#ifndef ACONFIG_ACONFIG_TM_H
#define ACONFIG_ACONFIG_TM_H

#include "osal.h"

/**
 * @brief 遥测功能通用枚举
 * @note 按功能类别分组，预留足够 ID 空间支持扩展
 */
typedef enum {
	/* 温度遥测类 (0-99) */
	ACONFIG_TM_CPU_TEMP = 0x0,
	ACONFIG_TM_BOARD_TEMP = 0x1,
	ACONFIG_TM_MCU_TEMP = 0x2,
	ACONFIG_TM_FPGA_TEMP = 0x3,
	ACONFIG_TM_FAN_SPEED = 0x4,

	/* 电压遥测类 (100-199) */
	ACONFIG_TM_VOLTAGE_12V = 0x64,
	ACONFIG_TM_VOLTAGE_5V = 0x65,
	ACONFIG_TM_VOLTAGE_3V3 = 0x66,
	ACONFIG_TM_CURRENT = 0x67,

	/* 状态遥测类 (200-299) */
	ACONFIG_TM_POWER_STATUS = 0xC8,
	ACONFIG_TM_MCU_STATUS = 0xC9,
	ACONFIG_TM_FPGA_STATUS = 0xCA,
	ACONFIG_TM_FPGA_CONFIG_STATUS = 0xCB,
	ACONFIG_TM_WATCHDOG_STATUS = 0xCC,

	/* 系统遥测类 (300-399) */
	ACONFIG_TM_SYSTEM_UPTIME = 0x12C,
	ACONFIG_TM_MCU_UPTIME = 0x12D,
	ACONFIG_TM_ERROR_COUNT = 0x12E,

	/* 预留扩展空间 */
	ACONFIG_TM_FUNC_MAX = 0x3E8
} aconfig_tm_function_t;

#endif /* ACONFIG_ACONFIG_TM_H */
