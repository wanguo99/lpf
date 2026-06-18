/************************************************************************
 * PCONFIG 通用类型定义
 *
 * 功能：
 * - GPIO配置（PCONFIG 层扩展功能）
 *
 * 说明：
 * - 本文件包含 PCONFIG 层特有的扩展配置类型
 * - 硬件接口类型由 PDL 层定义（pdl_mcu_interface_t）
 ************************************************************************/

#ifndef PCONFIG_COMMON_H
#define PCONFIG_COMMON_H

#include "osal.h"

/*===========================================================================
 * GPIO配置（PCONFIG 层扩展）
 *===========================================================================*/

/**
 * @brief GPIO配置
 *
 * 说明：
 * - 用于 PCONFIG 层的设备扩展（复位 GPIO、中断 GPIO 等）
 * - 实际 GPIO 操作通过 HAL 层实现
 */
typedef struct {
    uint32_t gpio_num; /* GPIO编号 */
    uint32_t pin_mux;  /* 引脚复用配置 */
    bool active_low;   /* 低电平有效 */
    bool pull_up;      /* 上拉使能 */
    bool pull_down;    /* 下拉使能 */
} pconfig_gpio_config_t;

#endif /* PCONFIG_COMMON_H */
