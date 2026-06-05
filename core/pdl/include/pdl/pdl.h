/************************************************************************
 * EMS - Embedded Middleware System
 *
 * 外设驱动层统一头文件
 *
 * 功能：
 * - 统一包含所有 PDL 对外头文件
 * - 提供一站式导入接口
 * - 简化应用层使用
 *
 * 使用方式：
 *   #include "pdl.h"  // 一次性导入所有 PDL API
 *
 * 设计原则:
 * 1. 按功能分类组织头文件
 * 2. 类型定义优先（pdl_types.h 最先包含）
 * 3. 设备驱动按字母顺序排列
 * 4. 保持与 OSAL 相同的组织风格
 ************************************************************************/

#ifndef PDL_H
#define PDL_H

/*
 * PDL 版本信息
 */
#define PDL_VERSION_MAJOR  0x1
#define PDL_VERSION_MINOR  0x0
#define PDL_VERSION_PATCH  0x0

/* 依赖的基础库 */
#include "osal.h"

/* 类型定义（必须最先包含，其他模块依赖它） */
#include "pdl_types.h"

/* 设备驱动 - 按字母顺序 */
#include "pdl_bmc.h"         /* BMC（基板管理控制器）驱动 */
#include "pdl_ccm.h"         /* CCM（通信管理板）驱动 */
#include "pdl_mcu.h"         /* MCU（微控制器）驱动 */
#include "pdl_satellite.h"   /* Satellite（卫星平台）驱动 */
#include "pdl_watchdog.h"    /* Watchdog（看门狗）驱动 */

#endif /* PDL_H */
