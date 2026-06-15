/************************************************************************
 * PCONFIG 通用类型定义
 *
 * 功能：
 * - GPIO配置
 * - 电源域配置
 * - 硬件接口类型枚举
 *
 * 说明：
 * - 本文件包含所有外设共用的基础类型定义
 ************************************************************************/

#ifndef PCONFIG_COMMON_H
#define PCONFIG_COMMON_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================
 * GPIO配置
 *===========================================================================*/

/**
 * @brief GPIO配置
 */
typedef struct {
	uint32_t gpio_num;		/* GPIO编号 */
	uint32_t pin_mux;		/* 引脚复用配置 */
	bool	active_low;		/* 低电平有效 */
	bool	pull_up;		/* 上拉使能 */
	bool	pull_down;		/* 下拉使能 */
} pconfig_gpio_config_t;

/*===========================================================================
 * 电源域配置
 *===========================================================================*/

/**
 * @brief 电源域配置
 */
typedef struct {
	const char *name;		/* 电源域名称 */
	pconfig_gpio_config_t *enable_gpio; /* 使能GPIO */
	uint32_t	voltage_mv;	/* 电压（mV） */
	uint32_t	current_ma;	/* 电流限制（mA） */
	uint32_t	startup_delay_ms; /* 启动延时（ms） */
} pconfig_power_domain_t;

/*===========================================================================
 * 硬件接口类型枚举
 *===========================================================================*/

/**
 * @brief 硬件接口类型枚举
 */
typedef enum {
	PCONFIG_HW_INTERFACE_NONE = 0,
	PCONFIG_HW_INTERFACE_CAN,
	PCONFIG_HW_INTERFACE_UART,
	PCONFIG_HW_INTERFACE_I2C,
	PCONFIG_HW_INTERFACE_SPI,
	PCONFIG_HW_INTERFACE_ETHERNET,
	PCONFIG_HW_INTERFACE_USB,
	PCONFIG_HW_INTERFACE_SPACEWIRE,
	PCONFIG_HW_INTERFACE_1553B,
	PCONFIG_HW_INTERFACE_MAX
} pconfig_hw_interface_type_t;

#endif /* PCONFIG_COMMON_H */
