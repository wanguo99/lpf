/************************************************************************
 * PCONFIG 内部总头文件
 *
 * 功能：
 * - PCONFIG 内部使用的总头文件
 * - 包含所有对外 API 和类型定义
 * - 包含内部工具头文件
 *
 * 使用方式：
 *   PCONFIG 内部源文件：#include "pconfig.h"
 *   外部模块（对外）：  #include "pconfig.h" (从 api/ 目录)
 ************************************************************************/

#ifndef PCONFIG_INTERNAL_H
#define PCONFIG_INTERNAL_H

/* 对外 API 和类型（从 api 目录） */
#include "pconfig_api.h"

/* 内部头文件（仅内部使用） */
#include "pconfig_common.h"
#include "pconfig_board.h"
#include "pconfig_hardware_interface.h"
#include "pconfig_mcu.h"
#include "pconfig_bmc.h"
#include "pconfig_fpga.h"
#include "pconfig_switch.h"

#endif /* PCONFIG_INTERNAL_H */
