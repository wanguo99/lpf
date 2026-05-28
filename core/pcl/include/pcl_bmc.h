/************************************************************************
 * PCL BMC外设配置
 *
 * 功能：
 * - BMC外设配置类型定义
 * - 完全匹配PDL层的pdl_bmc.h配置需求
 *
 * 说明：
 * - BMC作为基板管理控制器，使用IPMI/Redfish等管理协议
 * - 支持双通道：网络通道（主）+ 串口通道（备）
 * - 支持自动故障切换
 ************************************************************************/

#ifndef PCL_BMC_H
#define PCL_BMC_H

#include "pcl/pcl_common.h"

/*===========================================================================
 * BMC通道类型枚举（匹配PDL层）
 *===========================================================================*/

/**
 * @brief BMC通信通道类型
 */
typedef enum {
    PCL_BMC_CHANNEL_NETWORK = 0,  /* 网络通道（IPMI over LAN） */
    PCL_BMC_CHANNEL_SERIAL  = 1   /* 串口通道（IPMI over Serial） */
} pcl_bmc_channel_t;

/**
 * @brief BMC协议类型
 */
typedef enum {
    PCL_BMC_PROTOCOL_IPMI = 0,    /* IPMI协议 */
    PCL_BMC_PROTOCOL_REDFISH = 1  /* Redfish协议 */
} pcl_bmc_protocol_t;

/*===========================================================================
 * BMC外设配置（完全匹配PDL层bmc_config_t）
 *===========================================================================*/

/**
 * @brief BMC外设配置
 *
 * 此结构完全匹配PDL层的bmc_config_t，确保配置可以直接传递给PDL_BMC_Init()
 */
typedef struct {
    /* 外设基本信息（PCL层扩展字段） */
    const char *name;             /* BMC名称（如"payload_bmc"） */
    const char *description;      /* 描述信息 */
    bool        enabled;          /* 是否启用此BMC */

    /* 网络通道配置（匹配PDL层） */
    struct {
        bool        enabled;      /* 是否启用网络通道 */
        const char *ip_addr;      /* BMC IP地址 */
        uint16_t    port;         /* IPMI端口（默认623） */
        const char *username;     /* 用户名 */
        const char *password;     /* 密码 */
        uint32_t    timeout_ms;   /* 超时时间（ms） */
    } network;

    /* 串口通道配置（匹配PDL层） */
    struct {
        bool        enabled;      /* 是否启用串口通道 */
        const char *device;       /* 串口设备（如"/dev/ttyS0"） */
        uint32_t    baudrate;     /* 波特率 */
        uint32_t    timeout_ms;   /* 超时时间（ms） */
    } serial;

    /* 服务配置（匹配PDL层） */
    pcl_bmc_channel_t primary_channel;      /* 主通道（NETWORK/SERIAL） */
    bool              auto_switch;          /* 自动切换通道 */
    uint32_t          retry_count;          /* 重试次数 */
    uint32_t          health_check_interval;/* 健康检查间隔（ms） */

    /* GPIO控制（PCL层扩展字段，可选） */
    pcl_gpio_config_t *power_gpio;          /* 电源控制GPIO */
    pcl_gpio_config_t *reset_gpio;          /* 复位GPIO */
} pcl_bmc_cfg_t;

#endif /* PCL_BMC_H */
