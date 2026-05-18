/************************************************************************
 * PCL内部总头文件
 *
 * 功能：
 * - PCL内部使用的总头文件
 * - 包含所有公共头文件和外设头文件
 * - 对外只提供 pcl_api.h
 *
 * 使用方式：
 *   PCL内部源文件：#include "pcl.h"
 *   PDL层（对外）：  #include "pcl_api.h"
 ************************************************************************/

#ifndef PCL_H
#define PCL_H

/* 公共头文件 */
#include "pcl_common.h"
#include "pcl_board.h"

/* 硬件外设头文件 */
#include "pcl_hardware_interface.h"
#include "pcl_mcu.h"
#include "pcl_bmc.h"
#include "pcl_fpga.h"
#include "pcl_switch.h"

#endif /* PCL_H */
