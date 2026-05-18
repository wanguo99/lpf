/************************************************************************
 * 演示平台 - 演示项目载荷板基础配置（简化版）
 *
 * 平台：vendor_demo
 * 芯片：platform_demo  
 * 项目：project_demo
 * 产品：product_demo_base
 *
 * 设计原则：
 * - 只配置硬件外设（MCU、BMC、FPGA、Switch）
 * - 不包含业务逻辑、应用映射、传感器、存储等
 * - 配置以外设为单位，简洁明了
 ************************************************************************/

#include "internal/pcl.h"

/*===========================================================================
 * MCU外设配置数组
 *===========================================================================*/

static pcl_mcu_cfg_t mcu_stm32 = {
    .name = "stm32_mcu",
    .description = "STM32 MCU",
    .enabled = true,
    
    .interface_type = PCL_HW_INTERFACE_UART,
    .interface_cfg.uart = {
        .device = "/dev/ttyS1",
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 'N'
    },
    
    .cmd_timeout_ms = 500,
    .retry_count = 3,
    .enable_crc = true,
    
    .reset_gpio = NULL,
    .irq_gpio = NULL
};

static pcl_mcu_cfg_t *pcl_mcu_arr[] = {
    &mcu_stm32,
    NULL
};

/*===========================================================================
 * BMC外设配置数组
 *===========================================================================*/

static pcl_bmc_cfg_t bmc_payload = {
    .name = "payload_bmc",
    .description = "Payload BMC",
    .enabled = true,
    
    .primary_channel = {
        .protocol = PCL_BMC_PROTOCOL_IPMI,
        .cfg.ipmi_lan = {
            .interface = "eth0",
            .ip_addr = "192.168.1.100",
            .port = 623,
            .username = "admin",
            .password = NULL
        }
    },
    
    .backup_channel = {
        .protocol = PCL_BMC_PROTOCOL_IPMI,
        .cfg = {
            .device = "/dev/ttyS2",
            .baudrate = 115200,
            .data_bits = 8,
            .stop_bits = 1,
            .parity = 'N'
        }
    },
    
    .cmd_timeout_ms = 2000,
    .retry_count = 3,
    .failover_threshold = 5,
    
    .power_gpio = NULL,
    .reset_gpio = NULL
};

static pcl_bmc_cfg_t *pcl_bmc_arr[] = {
    &bmc_payload,
    NULL
};

/*===========================================================================
 * FPGA外设配置数组
 *===========================================================================*/

static pcl_fpga_cfg_t *pcl_fpga_arr[] = {
    NULL  /* 当前平台无FPGA */
};

/*===========================================================================
 * Switch外设配置数组
 *===========================================================================*/

static pcl_switch_cfg_t *pcl_switch_arr[] = {
    NULL  /* 当前平台无Switch */
};

/*===========================================================================
 * 平台配置注册
 *===========================================================================*/

pcl_platform_config_t platform_config_product_demo_base = {
    .platform_name = "vendor_demo",
    .chip_name = "platform_demo",
    .project_name = "project_demo",
    .product_name = "product_demo_base",
    
    .mcu_arr = pcl_mcu_arr,
    .bmc_arr = pcl_bmc_arr,
    .fpga_arr = pcl_fpga_arr,
    .switch_arr = pcl_switch_arr
};
