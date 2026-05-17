/************************************************************************
 * PCL卫星平台配置
 *
 * 功能：
 * - 卫星平台配置类型定义
 * - 完全匹配PDL层的pdl_satellite.h配置需求
 *
 * 说明：
 * - 卫星平台作为主控接口，使用CAN总线通信
 * - 支持命令接收、响应发送、心跳机制
 ************************************************************************/

#ifndef PCL_SATELLITE_H
#define PCL_SATELLITE_H

#include "pcl_common.h"

/*===========================================================================
 * 卫星平台配置（完全匹配PDL层satellite_service_config_t）
 *===========================================================================*/

/**
 * @brief 卫星平台配置
 *
 * 此结构完全匹配PDL层的satellite_service_config_t，
 * 确保配置可以直接传递给PDL_Satellite_Init()
 */
typedef struct {
    /* 外设基本信息（PCL层扩展字段） */
    const char *name;                   /* 卫星平台名称（如"satellite_bus"） */
    const char *description;            /* 描述信息 */
    bool        enabled;                /* 是否启用 */

    /* 卫星平台配置（匹配PDL层） */
    const char *can_device;             /* CAN设备名（如"can0"） */
    uint32_t    can_bitrate;            /* CAN波特率 */
    uint32_t    heartbeat_interval_ms;  /* 心跳间隔（ms） */
    uint32_t    cmd_timeout_ms;         /* 命令超时（ms） */

    /* GPIO控制（PCL层扩展字段，可选） */
    pcl_gpio_config_t *power_gpio;      /* 电源控制GPIO */
    pcl_gpio_config_t *reset_gpio;      /* 复位GPIO */
} pcl_satellite_cfg_t;

#endif /* PCL_SATELLITE_H */
