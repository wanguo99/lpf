/************************************************************************
 * PCL - Switch外设配置
 *
 * 功能：
 * - 网络交换机外设配置类型定义
 * - 支持多种管理接口（I2C、SPI、MDIO等）
 ************************************************************************/

#ifndef PCL_SWITCH_H
#define PCL_SWITCH_H

#include "pcl/pcl_hardware_interface.h"

/**
 * @brief Switch外设配置
 */
typedef struct {
    /* 外设基本信息 */
    const char *name;                    /* 外设名称 */
    const char *description;             /* 描述信息 */
    bool enabled;                        /* 是否启用 */

    /* 管理接口 */
    pcl_hw_interface_type_t interface_type;
    pcl_hw_interface_cfg_t interface_cfg;

    /* Switch特定配置 */
    uint32_t cmd_timeout_ms;             /* 命令超时时间 */
    uint32_t retry_count;                /* 重试次数 */
} pcl_switch_cfg_t;

#endif /* PCL_SWITCH_H */
