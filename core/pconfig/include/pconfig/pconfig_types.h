/************************************************************************
 * PCONFIG 对外类型定义
 *
 * 功能：
 * - 配置结构体和枚举类型
 * - GPIO配置、电源域配置
 * - 硬件接口类型枚举
 * - MCU/BMC/FPGA/Switch 配置条目
 * - 板级配置结构
 *
 * 说明：
 * - 本文件包含所有对外暴露的类型定义
 * - 对外 API 使用者只需包含 pconfig.h 或本文件
 ************************************************************************/

#ifndef PCONFIG_PCONFIG_TYPES_H
#define PCONFIG_PCONFIG_TYPES_H

#include "osal/osal.h"
#include "pdl/pdl_types_api.h"

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

/*===========================================================================
 * MCU外设配置条目
 *===========================================================================*/

/**
 * @brief MCU外设配置条目
 *
 * PCL 层只负责配置管理，实际配置结构由 types 模块定义
 */
typedef struct {
	/* PCL 配置管理字段 */
	const char *name;		/* MCU名称（用于查询，如"power_mcu"） */
	const char *description;	/* 描述信息 */
	bool	enabled;		/* 是否启用此MCU */

	/* PDL 配置（来自 types 模块） */
	pdl_mcu_config_t config;	/* MCU配置（来自 pdl_types.h） */

	/* GPIO控制（可选，PCL层扩展） */
	pconfig_gpio_config_t *reset_gpio;	/* 复位GPIO */
	pconfig_gpio_config_t *irq_gpio;	/* 中断GPIO */
} pconfig_mcu_entry_t;

/*===========================================================================
 * BMC外设配置条目
 *===========================================================================*/

/**
 * @brief BMC外设配置条目
 *
 * PCL 层只负责配置管理，实际配置结构由 types 模块定义
 */
typedef struct {
	/* PCL 配置管理字段 */
	const char *name;		/* BMC名称（用于查询，如"payload_bmc"） */
	const char *description;	/* 描述信息 */
	bool	enabled;		/* 是否启用此BMC */

	/* PDL 配置（来自 types 模块） */
	pdl_bmc_config_t config;	/* BMC配置（来自 pdl_types.h） */

	/* GPIO控制（可选，PCL层扩展） */
	pconfig_gpio_config_t *power_gpio;	/* 电源控制GPIO */
	pconfig_gpio_config_t *reset_gpio;	/* 复位GPIO */
} pconfig_bmc_entry_t;

/*===========================================================================
 * FPGA外设配置
 *===========================================================================*/

/**
 * @brief FPGA外设配置
 */
typedef struct {
	/* 外设基本信息 */
	const char *name;		/* 外设名称 */
	const char *description;	/* 描述信息 */
	bool enabled;			/* 是否启用 */

	/* FPGA特定配置 */
	const char *device;		/* 设备路径 */
	uint32_t cmd_timeout_ms;	/* 命令超时时间 */
	uint32_t retry_count;		/* 重试次数 */
} pconfig_fpga_cfg_t;

/*===========================================================================
 * Switch外设配置
 *===========================================================================*/

/**
 * @brief Switch外设配置
 */
typedef struct {
	/* 外设基本信息 */
	const char *name;		/* 外设名称 */
	const char *description;	/* 描述信息 */
	bool enabled;			/* 是否启用 */

	/* Switch特定配置 */
	const char *device;		/* 设备路径 */
	uint32_t cmd_timeout_ms;	/* 命令超时时间 */
	uint32_t retry_count;		/* 重试次数 */
} pconfig_switch_cfg_t;

/*===========================================================================
 * 板级配置（顶层）
 *===========================================================================*/

/**
 * @brief 板级硬件配置
 *
 * 这是顶层配置结构，以外设为单位描述整个板子的硬件配置
 * 只包含纯硬件外设：MCU、BMC、FPGA、Switch
 */
typedef struct {
	/* 板级信息 */
	const char *platform_name;	/* 平台名称（如"ti"） */
	const char *chip_name;		/* 芯片名称（如"am6254"） */
	const char *project_name;	/* 项目名称（如"H200_100P"） */
	const char *product_name;	/* 产品名称（如"h200_100p_base"） */

	/* 硬件外设配置数组（NULL结尾） */
	pconfig_mcu_entry_t	**mcu_arr;	/* MCU外设数组 */
	pconfig_bmc_entry_t	**bmc_arr;	/* BMC外设数组 */
	pconfig_fpga_cfg_t	**fpga_arr;	/* FPGA外设数组 */
	pconfig_switch_cfg_t	**switch_arr;	/* Switch外设数组 */
} pconfig_platform_config_t;

#endif /* PCONFIG_TYPES_H */
