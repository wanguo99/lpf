/************************************************************************
 * PCONFIG 平台配置类型定义
 *
 * 功能：
 * - 板级配置（顶层）
 *
 * 说明：
 * - 汇总所有外设配置，形成完整的板级配置
 ************************************************************************/

#ifndef PCONFIG_PLATFORM_H
#define PCONFIG_PLATFORM_H

#include "pconfig_mcu.h"
#include "pconfig_bmc.h"
#include "pconfig_fpga.h"
#include "pconfig_switch.h"
#include "pconfig_satellite.h"
#include "pconfig_ccm.h"
#include "pconfig_watchdog.h"

/*===========================================================================
 * 板级配置（顶层）
 *===========================================================================*/

/**
 * @brief 板级硬件配置
 *
 * 这是顶层配置结构，以外设为单位描述整个板子的硬件配置
 * 只包含纯硬件外设：MCU、BMC、FPGA、Switch
 *
 * 设计说明：
 * - 使用计数器+直接数组指针模式
 * - 配置数组直接定义，不使用指针数组
 * - 通过数组索引访问，业务层通过索引映射到具体硬件
 */
typedef struct {
	/* 板级信息 */
	const char *platform_name;	/* 平台名称（如"ti"） */
	const char *chip_name;		/* 芯片名称（如"am6254"） */
	const char *project_name;	/* 项目名称（如"H200_100P"） */
	const char *product_name;	/* 产品名称（如"h200_100p"） */

	/* 硬件外设配置数组（直接数组指针） */
	uint32_t mcu_count;		/* MCU外设数量 */
	pconfig_mcu_entry_t *mcu_array;	/* MCU外设数组（直接指向数组首元素） */

	uint32_t bmc_count;		/* BMC外设数量 */
	pconfig_bmc_entry_t *bmc_array;	/* BMC外设数组 */

	uint32_t fpga_count;		/* FPGA外设数量 */
	pconfig_fpga_cfg_t *fpga_array;	/* FPGA外设数组 */

	uint32_t switch_count;		/* Switch外设数量 */
	pconfig_switch_cfg_t *switch_array; /* Switch外设数组 */

	uint32_t satellite_count;	/* Satellite外设数量 */
	pconfig_satellite_entry_t *satellite_array; /* Satellite外设数组 */

	uint32_t ccm_count;		/* CCM外设数量 */
	pconfig_ccm_entry_t *ccm_array;	/* CCM外设数组 */

	uint32_t watchdog_count;	/* Watchdog外设数量 */
	pconfig_watchdog_entry_t *watchdog_array; /* Watchdog外设数组 */
} pconfig_platform_config_t;

#endif /* PCONFIG_PLATFORM_H */
