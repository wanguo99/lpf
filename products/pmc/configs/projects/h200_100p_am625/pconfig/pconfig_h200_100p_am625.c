/************************************************************************
 * TI AM625平台 - H200-100P-AM625载荷板配置
 *
 * 说明：
 * 本配置展示所有支持的硬件接口和通信方式
 * - MCU: CAN、UART、I2C（预留）、SPI（预留）
 * - BMC: IPMI/Redfish over Network/Serial
 * - 传感器通过 MCU 的 I2C/SPI 接口连接
 * - PMC 等外设通过 MCU 的 CAN/Ethernet 接口连接
 *
 * 设计原则：
 * - 配置按数组索引访问，不使用名称查找
 * - 业务层（ACONFIG）通过索引映射到具体硬件
 * - description 字段用于日志和调试
 ************************************************************************/

#include "osal.h"
#include "pconfig.h"

/* 辅助宏：计算数组元素数量 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*===========================================================================
 * MCU外设配置数组 - 按硬件接口索引，不关心业务角色
 *===========================================================================*/

static pconfig_mcu_entry_t pconfig_mcu_array[] = {
    /* [0] UART /dev/ttyS1 */
    {
        .description = "MCU on UART /dev/ttyS1 @ 115200bps",
        .enabled = true,
        .config = {
            .name = "uart_mcu_0",
            .interface = PCONFIG_MCU_INTERFACE_SERIAL,
            .hw.serial = {
                .device = "/dev/ttyS1",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = PCONFIG_MCU_PARITY_NONE,
                .flow_control = PCONFIG_MCU_FLOW_NONE
            },
            .cmd_timeout_ms = 500,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    },
    /* [1] CAN can0 ID:0x100/0x101 */
    {
        .description = "MCU on CAN can0 ID:0x100/0x101 @ 500kbps",
        .enabled = true,
        .config = {
            .name = "can_mcu_0",
            .interface = PCONFIG_MCU_INTERFACE_CAN,
            .hw.can = {
                .device = "can0",
                .bitrate = 500000,
                .rx_timeout = 100,
                .tx_timeout = 100,
                .tx_id = 0x100,
                .rx_id = 0x101
            },
            .cmd_timeout_ms = 1000,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    },
    /* [2] I2C (预留) */
    {
        .description = "MCU on I2C (Reserved)",
        .enabled = false,
        .config = {
            .name = "i2c_mcu_0",
            .interface = PCONFIG_MCU_INTERFACE_I2C,
            .hw = {{0}},
            .cmd_timeout_ms = 200,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    },
    /* [3] SPI (预留) */
    {
        .description = "MCU on SPI (Reserved)",
        .enabled = false,
        .config = {
            .name = "spi_mcu_0",
            .interface = PCONFIG_MCU_INTERFACE_SPI,
            .hw = {{0}},
            .cmd_timeout_ms = 100,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    },
    /* [4] CAN can0 ID:0x200/0x201 */
    {
        .description = "MCU on CAN can0 ID:0x200/0x201 @ 1Mbps",
        .enabled = false,
        .config = {
            .name = "can_mcu_1",
            .interface = PCONFIG_MCU_INTERFACE_CAN,
            .hw.can = {
                .device = "can0",
                .bitrate = 1000000,
                .rx_timeout = 100,
                .tx_timeout = 100,
                .tx_id = 0x200,
                .rx_id = 0x201
            },
            .cmd_timeout_ms = 500,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    },
    /* [5] UART /dev/ttyS3 */
    {
        .description = "MCU on UART /dev/ttyS3 @ 921600bps",
        .enabled = false,
        .config = {
            .name = "uart_mcu_1",
            .interface = PCONFIG_MCU_INTERFACE_SERIAL,
            .hw.serial = {
                .device = "/dev/ttyS3",
                .baudrate = 921600,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = PCONFIG_MCU_PARITY_NONE,
                .flow_control = PCONFIG_MCU_FLOW_NONE
            },
            .cmd_timeout_ms = 1000,
            .retry_count = 3
        },
        .reset_gpio = NULL,
        .irq_gpio = NULL
    }
};

/*===========================================================================
 * FPGA外设配置数组
 *===========================================================================*/

static pconfig_fpga_cfg_t pconfig_fpga_array[] = {
    /* [0] /dev/fpga0 */
    {
        .description = "FPGA at /dev/fpga0 (Custom logic)",
        .enabled = false,
        .device = "/dev/fpga0",
        .cmd_timeout_ms = 1000,
        .retry_count = 3
    }
};

/*===========================================================================
 * Switch外设配置数组
 *===========================================================================*/

static pconfig_switch_cfg_t pconfig_switch_array[] = {
    /* [0] /dev/switch0 */
    {
        .description = "Switch at /dev/switch0 (Ethernet routing)",
        .enabled = false,
        .device = "/dev/switch0",
        .cmd_timeout_ms = 500,
        .retry_count = 3
    }
};

/*===========================================================================
 * 平台配置注册
 *===========================================================================*/

const pconfig_platform_config_t pconfig_h200_100p_am625 = {
    .platform_name = "ti/am6254",
    .chip_name = "am6254",
    .project_name = "H200_100P_AM625",
    .product_name = "h200_100p_am625",

    /* MCU 外设：
     * [0] mcu_stm32_uart    - UART /dev/ttyS1
     * [1] mcu_power_can     - CAN can0 ID:0x100/0x101
     * [2] mcu_sensor_i2c    - I2C (预留)
     * [3] mcu_fpga_spi      - SPI (预留)
     * [4] mcu_pmc_can       - CAN can0 ID:0x200/0x201
     * [5] mcu_pmc_ethernet  - UART /dev/ttyS3
     */
    .mcu_count = ARRAY_SIZE(pconfig_mcu_array),
    .mcu_array = pconfig_mcu_array,

    /* FPGA 外设 */
    .fpga_count = ARRAY_SIZE(pconfig_fpga_array),
    .fpga_array = pconfig_fpga_array,

    /* Switch 外设 */
    .switch_count = ARRAY_SIZE(pconfig_switch_array),
    .switch_array = pconfig_switch_array
};

