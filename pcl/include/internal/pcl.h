/************************************************************************
 * PCL内部总头文件
 *
 * 功能：
 * - PCL内部使用的总头文件
 * - 包含所有内部公共头文件和外设头文件
 * - 对外只提供 pcl_api.h
 *
 * 使用方式：
 *   PCL内部源文件：#include "internal/pcl.h"
 *   PDL层（对外）：  #include "pcl_api.h"
 *
 * 目录结构：
 *   api/        - 对外API接口（仅pcl_api.h）
 *   internal/   - 内部公共头文件（含pcl.h）
 *   peripheral/ - 外设私有头文件
 ************************************************************************/

#ifndef PCL_H
#define PCL_H

/* 内部公共头文件 */
#include "pcl_common.h"
#include "pcl_board.h"

/* 硬件外设头文件（只包含纯硬件外设） */
#include "../peripheral/pcl_hardware_interface.h"
#include "../peripheral/pcl_mcu.h"
#include "../peripheral/pcl_bmc.h"
#include "../peripheral/pcl_fpga.h"
#include "../peripheral/pcl_switch.h"

#endif /* PCL_H */
