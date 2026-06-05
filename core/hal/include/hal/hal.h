/**
 * @file hal.h
 * @brief HAL (Hardware Abstraction Layer) - Unified API Entry Point
 * @details 硬件抽象层统一入口头文件
 *
 * 设计原则：
 * 1. 提供统一的硬件抽象接口
 * 2. 屏蔽底层硬件差异
 * 3. 支持多平台（Linux、RTOS）
 * 4. 遵循航空航天嵌入式标准
 */

#ifndef HAL_HAL_H
#define HAL_HAL_H

/*
 * HAL 版本信息
 */
#define HAL_VERSION_MAJOR  0x01
#define HAL_VERSION_MINOR  0x00
#define HAL_VERSION_PATCH  0x00

/* 基础类型定义 */
#include "hal_types.h"

/* 配置和类型定义 */
#include "config/hal_can_config.h"
#include "config/hal_can_types.h"
#include "config/hal_i2c_types.h"
#include "config/hal_spi_types.h"
#include "config/hal_uart_config.h"

/* HAL API - 按外设类型组织 */
#include "hal_can.h"
#include "hal_gpio.h"
#include "hal_i2c.h"
#include "hal_serial.h"
#include "hal_spi.h"
#include "hal_watchdog.h"

#endif /* HAL_HAL_H */
