/************************************************************************
 * PCL MCU外设配置
 *
 * 功能：
 * - MCU外设配置类型定义
 * - 完全匹配PDL层的pdl_mcu.h配置需求
 *
 * 说明：
 * - MCU作为硬件微控制器，使用物理层硬件接口通信
 * - 支持CAN/UART/I2C/SPI等多种硬件接口
 ************************************************************************/

#ifndef PCL_MCU_H
#define PCL_MCU_H

#include "pcl_common.h"

/*===========================================================================
 * MCU接口类型枚举（匹配PDL层）
 *===========================================================================*/

/**
 * @brief MCU通信接口类型
 */
typedef enum {
    PCL_MCU_INTERFACE_CAN = 0,     /* CAN总线 */
    PCL_MCU_INTERFACE_SERIAL = 1,  /* 串口 */
    PCL_MCU_INTERFACE_I2C = 2,     /* I2C（预留） */
    PCL_MCU_INTERFACE_SPI = 3      /* SPI（预留） */
} pcl_mcu_interface_t;

/*===========================================================================
 * MCU外设配置（完全匹配PDL层mcu_config_t）
 *===========================================================================*/

/**
 * @brief MCU外设配置
 *
 * 此结构完全匹配PDL层的mcu_config_t，确保配置可以直接传递给PDL_MCU_Init()
 */
typedef struct {
    /* 外设基本信息（PCL层扩展字段） */
    const char *pcl_name;           /* PCL层使用的名称（用于查询） */
    const char *description;        /* 描述信息 */
    bool        enabled;            /* 是否启用此MCU */

    /* MCU配置（匹配PDL层） */
    char name[64];                  /* MCU名称（传递给PDL） */
    pcl_mcu_interface_t interface;  /* 通信接口类型 */

    /* CAN配置 */
    struct {
        const char *device;         /* CAN设备（如can0） */
        uint32_t    bitrate;        /* 波特率 */
        uint32_t    tx_id;          /* 发送CAN ID */
        uint32_t    rx_id;          /* 接收CAN ID */
    } can;

    /* 串口配置 */
    struct {
        const char *device;         /* 串口设备（如/dev/ttyS1） */
        uint32_t    baudrate;       /* 波特率 */
        uint8_t     data_bits;      /* 数据位（5-8） */
        uint8_t     stop_bits;      /* 停止位（1-2） */
        int8_t      parity;         /* 校验位（'N'/'E'/'O'） */
    } serial;

    /* 通用配置（匹配PDL层） */
    uint32_t cmd_timeout_ms;        /* 命令超时（ms） */
    uint32_t retry_count;           /* 重试次数 */
    bool     enable_crc;            /* 启用CRC校验 */

    /* GPIO控制（PCL层扩展字段，可选） */
    pcl_gpio_config_t *reset_gpio;  /* 复位GPIO */
    pcl_gpio_config_t *irq_gpio;    /* 中断GPIO */
} pcl_mcu_cfg_t;

#endif /* PCL_MCU_H */
