/************************************************************************
 * XConfig硬件接口定义
 *
 * 功能：
 * - 硬件通信接口类型枚举（CAN/UART/I2C/SPI/Ethernet/USB/SpaceWire/1553B）
 * - 各硬件接口配置结构
 *
 * 说明：
 * - 本文件定义物理层硬件通信接口
 * - 主要被pcl_mcu.h、pcl_satellite.h等硬件外设使用
 * - BMC等软件层外设使用自己的协议配置（IPMI/Redfish等）
 ************************************************************************/

#ifndef PCL_HARDWARE_INTERFACE_H
#define PCL_HARDWARE_INTERFACE_H

#include "osal_types.h"
#include "hal_serial.h"  /* 串口校验位常量定义 */

/*===========================================================================
 * 硬件接口类型枚举
 *===========================================================================*/

/**
 * @brief 硬件接口类型枚举
 */
typedef enum {
    PCL_HW_INTERFACE_NONE = 0,
    PCL_HW_INTERFACE_CAN,
    PCL_HW_INTERFACE_UART,
    PCL_HW_INTERFACE_I2C,
    PCL_HW_INTERFACE_SPI,
    PCL_HW_INTERFACE_ETHERNET,
    PCL_HW_INTERFACE_USB,
    PCL_HW_INTERFACE_SPACEWIRE,
    PCL_HW_INTERFACE_1553B,
    PCL_HW_INTERFACE_MAX
} pcl_hw_interface_type_t;

/*===========================================================================
 * 硬件接口配置结构
 *===========================================================================*/

/**
 * @brief CAN接口配置
 */
typedef struct {
    const char *device;           /* 设备名（如"can0"） */
    uint32_t      bitrate;          /* 波特率（bps） */
    uint32_t      tx_id;            /* 发送CAN ID */
    uint32_t      rx_id;            /* 接收CAN ID */
} pcl_can_cfg_t;

/**
 * @brief UART接口配置
 */
typedef struct {
    const char *device;          /* 设备名（如"/dev/ttyS0"） */
    uint32_t       baudrate;        /* 波特率 */
    uint8_t        data_bits;       /* 数据位（5-8） */
    uint8_t        stop_bits;       /* 停止位（1-2） */
    uint8_t        parity;          /* 校验位（HAL_SERIAL_PARITY_NONE/ODD/EVEN） */
} pcl_uart_cfg_t;

/**
 * @brief I2C接口配置
 */
typedef struct {
    const char *device;           /* 设备名（如"/dev/i2c-0"） */
    uint8_t       slave_addr;       /* 从设备地址（7位） */
    uint32_t      speed_hz;         /* 速度（Hz） */
} pcl_i2c_cfg_t;

/**
 * @brief SPI接口配置
 */
typedef struct {
    const char *device;           /* 设备名（如"/dev/spidev0.0"） */
    uint32_t      speed_hz;         /* 时钟频率（Hz） */
    uint8_t       mode;             /* SPI模式（0-3） */
    uint8_t       bits_per_word;    /* 每字位数 */
} pcl_spi_cfg_t;

/**
 * @brief Ethernet接口配置
 */
typedef struct {
    const char *interface;        /* 接口名（如"eth0"） */
    const char *ip_addr;          /* IP地址 */
    uint16_t      port;             /* 端口号 */
} pcl_ethernet_cfg_t;

/**
 * @brief USB接口配置
 */
typedef struct {
    const char *device;           /* 设备名（如"/dev/ttyUSB0"） */
    uint16_t      vendor_id;        /* 厂商ID */
    uint16_t      product_id;       /* 产品ID */
} pcl_usb_cfg_t;

/**
 * @brief SpaceWire接口配置
 */
typedef struct {
    const char *device;           /* 设备名 */
    uint32_t      link_speed;       /* 链路速度（Mbps） */
} pcl_spacewire_cfg_t;

/**
 * @brief 1553B接口配置
 */
typedef struct {
    const char *device;           /* 设备名 */
    uint8_t       rt_address;       /* RT地址（0-31） */
} pcl_1553b_cfg_t;

/*===========================================================================
 * 硬件接口配置联合体
 *===========================================================================*/

/**
 * @brief 硬件接口配置联合体
 */
typedef union {
    pcl_can_cfg_t       can;
    pcl_uart_cfg_t      uart;
    pcl_i2c_cfg_t       i2c;
    pcl_spi_cfg_t       spi;
    pcl_ethernet_cfg_t  ethernet;
    pcl_usb_cfg_t       usb;
    pcl_spacewire_cfg_t spacewire;
    pcl_1553b_cfg_t     bus1553b;
} pcl_hw_interface_cfg_t;

#endif /* PCL_HARDWARE_INTERFACE_H */
