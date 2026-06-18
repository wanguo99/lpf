/************************************************************************
 * ES-Middleware - Embedded Software - Middleware
 *
 * 外设驱动层统一头文件
 *
 * 功能：
 * - 统一包含所有 PDM 对外头文件
 * - 提供一站式导入接口
 * - 简化应用层使用
 *
 * 使用方式：
 *   #include "pdm.h"  // 一次性导入所有 PDM API
 *
 * 设计原则:
 * 1. 按功能分类组织头文件
 * 2. 各模块头文件独立定义类型，无循环依赖
 * 3. 设备驱动按字母顺序排列
 * 4. 保持与 OSAL 相同的组织风格
 ************************************************************************/

#ifndef PDM_H
#define PDM_H

/*
 * PDM 版本信息
 */
#define PDM_VERSION_MAJOR 0x1
#define PDM_VERSION_MINOR 0x0
#define PDM_VERSION_PATCH 0x0

/* 依赖的基础库 */
#ifdef CONFIG_OSAL
#include "osal.h"
#endif /* CONFIG_OSAL */

/* 设备驱动 - 当前仅保留 MCU 外设类型 */
#ifdef CONFIG_PDM_MCU_SUPPORT
#include "pdm_mcu.h" /* MCU（微控制器）驱动 */
#endif /* CONFIG_PDM_MCU_SUPPORT */

#endif /* PDM_H */
