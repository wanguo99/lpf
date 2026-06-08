/************************************************************************
 * TI AM625平台 - H200-100P-AM625载荷板基础配置
 ************************************************************************/

#include "pconfig.h"
#include "hal.h"  /* 需要 HAL_SERIAL_* 常量 */

/*===========================================================================
 * MCU外设配置数组
 *===========================================================================*/

static pconfig_mcu_entry_t mcu_stm32 = {
    .name = "stm32_mcu",
    .description = "STM32 MCU for payload control",
    .enabled = true,

    .config = {
        .name = "stm32_mcu",
        .interface = PDL_MCU_INTERFACE_SERIAL,

        .hw.serial = {
            .device = "/dev/ttyS1",
            .baudrate = 115200,
            .data_bits = 8,
            .stop_bits = 1,
            .parity = HAL_SERIAL_PARITY_NONE,
            .flow_control = HAL_SERIAL_FLOW_NONE
        },

        .cmd_timeout_ms = 500,
        .retry_count = 3
        /* 注意：串口通信强制启用 CRC 校验 */
    },

    .reset_gpio = NULL,
    .irq_gpio = NULL
};

static pconfig_mcu_entry_t *pconfig_mcu_arr[] = {
    &mcu_stm32,
    NULL
};

/*===========================================================================
 * BMC外设配置数组
 *===========================================================================*/

static pconfig_bmc_entry_t bmc_payload = {
    .name = "payload_bmc",
    .description = "Payload BMC",
    .enabled = true,

    .config = {
        /* 主通道配置：网络 */
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .primary_config = {
            .network = {
                .ip_addr = "192.168.1.100",
                .port = 623,
                .username = "admin",
                .password = NULL,
                .timeout_ms = 2000
            }
        },

        /* 备用通道配置：串口 */
        .backup_channel = PDL_BMC_CHANNEL_SERIAL,
        .backup_config = {
            .serial = {
                .device = "/dev/ttyS2",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = HAL_SERIAL_PARITY_NONE,
                .timeout_ms = 2000
            }
        },

        /* 服务配置 */
        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 5000
    },

    .power_gpio = NULL,
    .reset_gpio = NULL
};

static pconfig_bmc_entry_t *pconfig_bmc_arr[] = {
    &bmc_payload,
    NULL
};

/*===========================================================================
 * FPGA外设配置数组
 *===========================================================================*/

static pconfig_fpga_cfg_t *pconfig_fpga_arr[] = {
    NULL
};

/*===========================================================================
 * Switch外设配置数组
 *===========================================================================*/

static pconfig_switch_cfg_t *pconfig_switch_arr[] = {
    NULL
};

/*===========================================================================
 * 平台配置注册
 *===========================================================================*/

/* 辅助宏：计算指针数组中有效元素数量（排除NULL结尾） */
#define ARRAY_COUNT(arr) ((sizeof(arr) / sizeof(arr[0])) - 1)

const pconfig_platform_config_t pconfig_h200_100p_am625_base = {
    .platform_name = "ti/am6254",
    .chip_name = "am6254",
    .project_name = "H200_100P_AM625",
    .product_name = "h200_100p_am625_base",

    .mcu_count = 1,  /* mcu_stm32 */
    .mcu_arr = pconfig_mcu_arr,

    .bmc_count = 1,  /* bmc_payload */
    .bmc_arr = pconfig_bmc_arr,

    .fpga_count = 0,  /* 空数组 */
    .fpga_arr = pconfig_fpga_arr,

    .switch_count = 0,  /* 空数组 */
    .switch_arr = pconfig_switch_arr
};
