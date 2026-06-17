/************************************************************************
 * ES-Middleware - Embedded Software - Middleware
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
 * 2. 各模块头文件独立定义类型，无循环依赖
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
#ifdef CONFIG_OSAL
#include "osal.h"
#endif /* CONFIG_OSAL */

/* 设备驱动 - 按字母顺序 */
#ifdef CONFIG_PDL_BMC_SUPPORT
#include "pdl_bmc.h"         /* BMC（基板管理控制器）驱动 */
#endif /* CONFIG_PDL_BMC_SUPPORT */

#ifdef CONFIG_PDL_CCM_SUPPORT
#include "pdl_ccm.h"         /* CCM（通信管理板）驱动 */
#endif /* CONFIG_PDL_CCM_SUPPORT */

#ifdef CONFIG_PDL_MCU_SUPPORT
#include "pdl_mcu.h"         /* MCU（微控制器）驱动 */
#endif /* CONFIG_PDL_MCU_SUPPORT */

#ifdef CONFIG_PDL_SATELLITE_SUPPORT
#include "pdl_satellite.h"   /* Satellite（卫星平台）驱动 */
#endif /* CONFIG_PDL_SATELLITE_SUPPORT */

#endif /* PDL_H */
