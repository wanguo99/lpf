/************************************************************************
 * XConfig通用类型定义
 *
 * 功能：
 * - GPIO配置
 * - 电源域配置
 *
 * 说明：
 * - 本文件定义所有外设配置共用的基础类型
 * - 被所有xconfig头文件引用
 ************************************************************************/

#ifndef PCL_COMMON_H
#define PCL_COMMON_H

#include "osal_types.h"

/*===========================================================================
 * GPIO配置
 *===========================================================================*/

/**
 * @brief GPIO配置
 */
typedef struct {
    uint32_t gpio_num;              /* GPIO编号 */
    uint32_t pin_mux;               /* 引脚复用配置 */
    bool   active_low;            /* 低电平有效 */
    bool   pull_up;               /* 上拉使能 */
    bool   pull_down;             /* 下拉使能 */
} pcl_gpio_config_t;

/*===========================================================================
 * 电源域配置
 *===========================================================================*/

/**
 * @brief 电源域配置
 */
typedef struct {
    const char *name;             /* 电源域名称 */
    pcl_gpio_config_t *enable_gpio; /* 使能GPIO */
    uint32_t      voltage_mv;       /* 电压（mV） */
    uint32_t      current_ma;       /* 电流限制（mA） */
    uint32_t      startup_delay_ms; /* 启动延时（ms） */
} pcl_power_domain_t;

#endif /* PCL_COMMON_H */
