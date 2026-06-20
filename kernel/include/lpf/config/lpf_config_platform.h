/************************************************************************
 * LPF_CONFIG 平台配置类型定义
 *
 * 功能：
 * - 板级配置（顶层）
 *
 * 说明：
 * - 汇总所有外设配置，形成完整的板级配置
 ************************************************************************/

#ifndef LPF_CONFIG_PLATFORM_H
#define LPF_CONFIG_PLATFORM_H

#include "lpf/config/lpf_config_mcu.h"
#include "lpf/config/lpf_config_led.h"

/*===========================================================================
 * 板级配置（顶层）
 *===========================================================================*/

/**
 * @brief 板级硬件配置
 *
 * 这是顶层配置结构，用 DTS-like configured-device node 表描述板级硬件。
 * 旧的按外设数组字段保留为兼容路径，新增静态配置应优先填写
 * device_nodes。
 *
 * 设计说明：
 * - device_nodes 是首选的有序配置节点表
 * - mcu_array/led_array 保留给旧调用者和过渡后端
 * - 运行时配置驱动消费 device node payload，不感知配置来源
 */
typedef struct {
	/* 板级信息 */
	const char *platform_name; /* 平台名称（如"ti"） */
	const char *chip_name; /* 芯片名称（如"am6254"） */
	const char *project_name; /* 项目名称（如"H200_100P"） */
	const char *product_name; /* 产品名称（如"framework"） */
	const char *version; /* 配置版本（如"1.0.0"） */

	/* 首选 DTS-like configured-device node 表 */
	uint32_t device_node_count; /* configured-device node 数量 */
	const lpf_config_device_node_t *device_nodes; /* 有序 node 表 */

	/* 兼容硬件外设配置数组（直接数组指针） */
	uint32_t mcu_count; /* MCU外设数量 */
	const lpf_config_mcu_entry_t *mcu_array; /* MCU外设数组（直接指向数组首元素） */
	uint32_t led_count; /* LED外设数量 */
	const lpf_config_led_entry_t *led_array; /* LED外设数组 */
} lpf_config_platform_config_t;

#endif /* LPF_CONFIG_PLATFORM_H */
