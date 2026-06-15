/************************************************************************
 * PCONFIG 对外类型定义
 *
 * 功能：
 * - 配置结构体和枚举类型
 * - GPIO配置、电源域配置
 * - 硬件接口类型枚举
 * - MCU/BMC/FPGA/Switch 配置条目
 * - 板级配置结构
 *
 * 说明：
 * - 本文件包含所有对外暴露的类型定义
 * - 对外 API 使用者只需包含 pconfig.h 或本文件
 * - 类型定义已按模块拆分到各子头文件
 ************************************************************************/

#ifndef PCONFIG_PCONFIG_TYPES_H
#define PCONFIG_PCONFIG_TYPES_H

/* 通用类型定义（GPIO、电源域、硬件接口枚举） */
#include "pconfig_common.h"

/* 外设配置类型定义 */
#include "pconfig_mcu.h"
#include "pconfig_bmc.h"
#include "pconfig_fpga.h"
#include "pconfig_switch.h"

/* 板级配置（顶层） */
#include "pconfig_platform.h"

#endif /* PCONFIG_PCONFIG_TYPES_H */
