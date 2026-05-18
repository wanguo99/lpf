/************************************************************************
 * TI AM625平台 - H200-100P载荷板基础配置
 ************************************************************************/

#include "pcl_config.h"

/*===========================================================================
 * MCU外设配置数组
 *===========================================================================*/

static pcl_mcu_cfg_t mcu_stm32 = {
    .pcl_name = "stm32_mcu",
    .description = "STM32 MCU for payload control",
    .enabled = true,
    
    .name = "stm32_mcu",
    .interface = PCL_MCU_INTERFACE_SERIAL,
    
    .serial = {
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
    
    .network = {
        .enabled = true,
        .ip_addr = "192.168.1.100",
        .port = 623,
        .username = "admin",
        .password = NULL,
        .timeout_ms = 2000
    },
    
    .serial = {
        .enabled = true,
        .device = "/dev/ttyS2",
        .baudrate = 115200,
        .timeout_ms = 2000
    },
    
    .primary_channel = PCL_BMC_CHANNEL_NETWORK,
    .auto_switch = true,
    .retry_count = 3,
    .health_check_interval = 5000,
    
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
    NULL
};

/*===========================================================================
 * Switch外设配置数组
 *===========================================================================*/

static pcl_switch_cfg_t *pcl_switch_arr[] = {
    NULL
};

/*===========================================================================
 * 平台配置注册
 *===========================================================================*/

const pcl_platform_config_t pcl_h200_100p_base = {
    .platform_name = "ti/am6254",
    .chip_name = "am6254",
    .project_name = "H200_100P",
    .product_name = "h200_100p_base",
    
    .mcu_arr = pcl_mcu_arr,
    .bmc_arr = pcl_bmc_arr,
    .fpga_arr = pcl_fpga_arr,
    .switch_arr = pcl_switch_arr
};
