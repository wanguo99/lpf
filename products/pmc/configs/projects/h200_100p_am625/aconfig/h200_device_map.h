/************************************************************************
 * H200 设备映射定义
 *
 * 功能：
 * - 定义业务功能到硬件设备的索引映射
 * - 实现配置与业务的解耦
 *
 * 设计原则：
 * - PCONFIG 只描述硬件存在什么（按索引）
 * - ACONFIG 定义业务需要什么（通过索引映射）
 * - 同一硬件配置可支持不同的业务映射
 ************************************************************************/

#ifndef H200_DEVICE_MAP_H
#define H200_DEVICE_MAP_H

#include <stdint.h>

/*===========================================================================
 * 设备索引映射结构
 *===========================================================================*/

/**
 * @brief TC (遥测与控制) 业务设备映射
 *
 * 将 PCONFIG 硬件索引映射到具体业务功能
 */
typedef struct {
	/* MCU 设备映射 */
	uint32_t payload_mcu_index;	/* 载荷控制 MCU */
	uint32_t power_mcu_index;	/* 电源管理 MCU */
	uint32_t thermal_mcu_index;	/* 热控 MCU（可选）*/

	/* BMC 设备映射 */
	uint32_t main_bmc_index;	/* 主 BMC */

	/* FPGA 设备映射 */
	uint32_t main_fpga_index;	/* 主 FPGA */

	/* Switch 设备映射 */
	uint32_t main_switch_index;	/* 主交换机 */
} h200_tc_device_map_t;

/**
 * @brief 健康监控业务设备映射
 */
typedef struct {
	/* 监控所有 MCU 健康状态 */
	uint32_t mcu_count;		/* 需要监控的 MCU 数量 */
	uint32_t *mcu_indices;		/* MCU 索引数组 */

	/* 监控所有 BMC 健康状态 */
	uint32_t bmc_count;		/* 需要监控的 BMC 数量 */
	uint32_t *bmc_indices;		/* BMC 索引数组 */
} h200_health_device_map_t;

/*===========================================================================
 * H200 平台设备映射配置
 *===========================================================================*/

/**
 * @brief 获取 H200 平台的 TC 设备映射
 *
 * @return TC 设备映射指针
 */
const h200_tc_device_map_t* H200_GetTCDeviceMap(void);

/**
 * @brief 获取 H200 平台的健康监控设备映射
 *
 * @return 健康监控设备映射指针
 */
const h200_health_device_map_t* H200_GetHealthDeviceMap(void);

#endif /* H200_DEVICE_MAP_H */
