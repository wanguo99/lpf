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
#include "pdl_misc.h"

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
 * - 使用计数器+指针数组模式，避免NULL结尾导致的越界风险
 * - 数组大小明确，便于边界检查和快速获取数量
 */
typedef struct {
	/* 板级信息 */
	const char *platform_name;	/* 平台名称（如"ti"） */
	const char *chip_name;		/* 芯片名称（如"am6254"） */
	const char *project_name;	/* 项目名称（如"H200_100P"） */
	const char *product_name;	/* 产品名称（如"h200_100p"） */

	/* 硬件ID支持 */
	uint32_t hwid_count;		/* 支持的HWID数量，0表示支持所有HWID */
	const pdl_hwid_t *hwid_list;	/* 支持的HWID列表，NULL表示支持所有HWID */

	/* 硬件外设配置数组（使用计数器模式） */
	uint32_t mcu_count;			/* MCU外设数量 */
	pconfig_mcu_entry_t	**mcu_arr;	/* MCU外设数组 */

	uint32_t bmc_count;			/* BMC外设数量 */
	pconfig_bmc_entry_t	**bmc_arr;	/* BMC外设数组 */

	uint32_t fpga_count;			/* FPGA外设数量 */
	pconfig_fpga_cfg_t	**fpga_arr;	/* FPGA外设数组 */

	uint32_t switch_count;			/* Switch外设数量 */
	pconfig_switch_cfg_t	**switch_arr;	/* Switch外设数组 */
} pconfig_platform_config_t;

#endif /* PCONFIG_PLATFORM_H */
