/************************************************************************
 * H200 设备映射实现
 *
 * 说明：
 * - 定义 H200 平台的业务到硬件的索引映射
 * - 映射关系与 PCONFIG 中的硬件配置数组对应
 ************************************************************************/

#include "h200_device_map.h"

/*===========================================================================
 * TC 业务设备映射
 *===========================================================================*/

/**
 * H200 平台 TC 设备映射
 *
 * 对应 PCONFIG 中的硬件配置：
 * - MCU[0]: UART /dev/ttyS1        -> 载荷控制
 * - MCU[1]: CAN can0 ID:0x100/0x101 -> 电源管理
 * - MCU[2]: I2C (预留)             -> 热控（未启用）
 * - BMC[0]: 192.168.1.100:623      -> 主 BMC
 * - FPGA[0]: /dev/fpga0            -> 主 FPGA
 * - Switch[0]: /dev/switch0        -> 主交换机
 */
static const h200_tc_device_map_t h200_tc_device_map = {
	/* MCU 映射 */
	.payload_mcu_index = 0,		/* 使用 MCU[0] 作为载荷控制 */
	.power_mcu_index = 1,		/* 使用 MCU[1] 作为电源管理 */
	.thermal_mcu_index = 2,		/* 使用 MCU[2] 作为热控（当前未启用）*/

	/* BMC 映射 */
	.main_bmc_index = 0,		/* 使用 BMC[0] 作为主 BMC */

	/* FPGA 映射 */
	.main_fpga_index = 0,		/* 使用 FPGA[0] */

	/* Switch 映射 */
	.main_switch_index = 0,		/* 使用 Switch[0] */
};

const h200_tc_device_map_t* H200_GetTCDeviceMap(void)
{
	return &h200_tc_device_map;
}

/*===========================================================================
 * 健康监控业务设备映射
 *===========================================================================*/

/* 需要监控的 MCU 索引列表 */
static uint32_t health_mcu_indices[] = {
	0,	/* MCU[0]: 载荷控制 */
	1,	/* MCU[1]: 电源管理 */
};

/* 需要监控的 BMC 索引列表 */
static uint32_t health_bmc_indices[] = {
	0,	/* BMC[0]: 主 BMC */
};

/**
 * H200 平台健康监控设备映射
 *
 * 监控所有启用的关键设备
 */
static const h200_health_device_map_t h200_health_device_map = {
	/* 监控 MCU */
	.mcu_count = sizeof(health_mcu_indices) / sizeof(health_mcu_indices[0]),
	.mcu_indices = health_mcu_indices,

	/* 监控 BMC */
	.bmc_count = sizeof(health_bmc_indices) / sizeof(health_bmc_indices[0]),
	.bmc_indices = health_bmc_indices,
};

const h200_health_device_map_t* H200_GetHealthDeviceMap(void)
{
	return &h200_health_device_map;
}
