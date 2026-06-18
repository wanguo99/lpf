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

/*===========================================================================
 * 板级配置（顶层）
 *===========================================================================*/

/**
 * @brief 板级硬件配置
 *
 * 这是顶层配置结构，以外设为单位描述整个板子的硬件配置
 * 当前只保留 MCU 外设类型，后续可按需继续增加其他外设
 *
 * 设计说明：
 * - 使用计数器+直接数组指针模式
 * - 配置数组直接定义，不使用指针数组
 * - 通过数组索引访问，业务层通过索引映射到具体硬件
 */
typedef struct {
	/* 板级信息 */
	const char *platform_name; /* 平台名称（如"ti"） */
	const char *chip_name; /* 芯片名称（如"am6254"） */
	const char *project_name; /* 项目名称（如"H200_100P"） */
	const char *product_name; /* 产品名称（如"framework"） */

	/* 硬件外设配置数组（直接数组指针） */
	uint32_t mcu_count; /* MCU外设数量 */
	const pconfig_mcu_entry_t *mcu_array; /* MCU外设数组（直接指向数组首元素） */
} pconfig_platform_config_t;

typedef struct {
	const pconfig_platform_config_t *const *configs;
	uint32_t count;
	uint32_t current_index;
} pconfig_platform_table_t;

#endif /* PCONFIG_PLATFORM_H */
