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
#define HAL_VERSION_MAJOR 0x01
#define HAL_VERSION_MINOR 0x00
#define HAL_VERSION_PATCH 0x00

/* 基础类型定义 - 所有 HAL 模块都需要 */
#include "hal_types.h"

/* CAN 总线 */
#ifdef CONFIG_HAL_CAN
#include "config/hal_can_types.h"
#include "hal_can.h"
#endif /* CONFIG_HAL_CAN */

/* GPIO */
#ifdef CONFIG_HAL_GPIO
#include "hal_gpio.h"
#endif /* CONFIG_HAL_GPIO */

/* I2C 总线 */
#ifdef CONFIG_HAL_I2C
#include "config/hal_i2c_types.h"
#include "hal_i2c.h"
#endif /* CONFIG_HAL_I2C */

/* SPI 总线 */
#ifdef CONFIG_HAL_SPI
#include "config/hal_spi_types.h"
#include "hal_spi.h"
#endif /* CONFIG_HAL_SPI */

/* UART/Serial */
#ifdef CONFIG_HAL_UART
#include "config/hal_uart_config.h"
#include "hal_serial.h"
#endif /* CONFIG_HAL_UART */

#endif /* HAL_HAL_H */
