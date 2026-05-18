/**
 * @file pcl_config.h
 * @brief PCL配置总入口头文件
 * @note 包含所有配置相关的类型和定义
 */

#ifndef PCL_CONFIG_H
#define PCL_CONFIG_H

/* 基础类型 */
#include "pcl_common.h"

/* 硬件接口 */
#include "pcl_hardware_interface.h"

/* 外设配置 */
#include "pcl_mcu.h"
#include "pcl_bmc.h"
#include "pcl_fpga.h"
#include "pcl_switch.h"

/* 板级配置 */
#include "pcl_board.h"

#endif /* PCL_CONFIG_H */
