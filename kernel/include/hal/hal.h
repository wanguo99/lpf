/**
 * @file hal.h
 * @brief HAL (Hardware Abstraction Layer) - Unified API Entry Point
 * @details 硬件抽象层统一入口头文件
 *
 * 设计原则：
 * 1. 提供统一的硬件抽象接口
 * 2. 屏蔽底层硬件差异
 * 3. 面向 Linux 内核态 LPF 外设服务
 * 4. 保持硬件访问边界清晰
 */

#ifndef HAL_HAL_H
#define HAL_HAL_H

/*
 * HAL 版本信息
 */
#define HAL_VERSION_MAJOR 0x01
#define HAL_VERSION_MINOR 0x00
#define HAL_VERSION_PATCH 0x00

void hal_print_version(void);
int32_t hal_runtime_init(void);
void hal_runtime_exit(void);

/* 基础类型定义 - 所有 HAL 模块都需要 */
#include "hal_types.h"

/* CAN 总线 */
#include "config/hal_can_types.h"
#include "hal_can.h"

/* GPIO */
#include "hal_gpio.h"

/* PWM */
#include "hal_pwm.h"

/* I2C 总线 */
#include "config/hal_i2c_types.h"
#include "hal_i2c.h"

/* SPI 总线 */
#include "config/hal_spi_types.h"
#include "hal_spi.h"

/* UART/Serial */
#include "config/hal_uart_config.h"
#include "hal_serial.h"

#endif /* HAL_HAL_H */
