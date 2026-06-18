/************************************************************************
 * PCONFIG MCU 配置类型定义
 *
 * 功能：
 * - MCU外设配置类型（完整定义）
 *
 * 说明：
 * - 配置类型由 PCONFIG 定义，PDL 使用
 * - 避免循环依赖
 ************************************************************************/

#ifndef PCONFIG_MCU_H
#define PCONFIG_MCU_H

#include "pconfig_common.h"

/*===========================================================================
 * MCU 配置类型定义
 *===========================================================================*/

/**
 * @brief MCU通信接口类型
 */
typedef enum {
    PCONFIG_MCU_INTERFACE_CAN = 0x00,    /* CAN总线 */
    PCONFIG_MCU_INTERFACE_SERIAL = 0x01, /* 串口 */
    PCONFIG_MCU_INTERFACE_I2C = 0x02,    /* I2C（预留） */
    PCONFIG_MCU_INTERFACE_SPI = 0x03     /* SPI（预留） */
} pconfig_mcu_interface_t;

/**
 * @brief 串口校验位类型
 */
typedef enum {
    PCONFIG_MCU_PARITY_NONE = 0, /* 无校验 */
    PCONFIG_MCU_PARITY_ODD,      /* 奇校验 */
    PCONFIG_MCU_PARITY_EVEN      /* 偶校验 */
} pconfig_mcu_parity_t;

/**
 * @brief 串口流控类型
 */
typedef enum {
    PCONFIG_MCU_FLOW_NONE = 0, /* 无流控 */
    PCONFIG_MCU_FLOW_HW,       /* 硬件流控（RTS/CTS） */
    PCONFIG_MCU_FLOW_SW        /* 软件流控（XON/XOFF） */
} pconfig_mcu_flow_control_t;

/**
 * @brief MCU配置（完整定义）
 */
typedef struct {
    char name[64];                     /* MCU名称 */
    pconfig_mcu_interface_t interface; /* 通信接口类型 */

    /* 硬件接口配置（Tagged Union） */
    union {
        /* CAN配置 */
        struct {
            const char *device;  /* CAN设备（如can0） */
            uint32_t bitrate;    /* 波特率 */
            uint32_t rx_timeout; /* 接收超时 */
            uint32_t tx_timeout; /* 发送超时 */
            uint32_t tx_id;      /* 发送CAN ID */
            uint32_t rx_id;      /* 接收CAN ID */
        } can;

        /* 串口配置 */
        struct {
            const char *device;          /* 串口设备（如/dev/ttyS1） */
            uint32_t baudrate;           /* 波特率 */
            uint8_t data_bits;           /* 数据位（5-8） */
            uint8_t stop_bits;           /* 停止位（1-2） */
            pconfig_mcu_parity_t parity; /* 校验位 */
            pconfig_mcu_flow_control_t flow_control; /* 流控 */
        } serial;
    } hw;

    /* 通用配置 */
    uint32_t cmd_timeout_ms; /* 命令超时（ms） */
    uint32_t retry_count;    /* 重试次数 */
} pconfig_mcu_config_t;

/**
 * @brief MCU外设配置条目
 */
typedef struct {
    const char *description;     /* 描述信息 */
    bool enabled;                /* 是否启用 */
    pconfig_mcu_config_t config; /* MCU配置 */

    /* GPIO控制（可选） */
    const pconfig_gpio_config_t *reset_gpio; /* 复位GPIO */
    const pconfig_gpio_config_t *irq_gpio;   /* 中断GPIO */
} pconfig_mcu_entry_t;

#endif /* PCONFIG_MCU_H */
