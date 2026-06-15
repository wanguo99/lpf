/************************************************************************
 * TI AM625平台 - H200-100P-AM625载荷板配置
 *
 * 说明：
 * 本配置展示所有支持的硬件接口和通信方式
 * - MCU: CAN、UART、I2C（预留）、SPI（预留）
 * - BMC: IPMI/Redfish over Network/Serial
 * - 传感器通过 MCU 的 I2C/SPI 接口连接
 * - PMC 等外设通过 MCU 的 CAN/Ethernet 接口连接
 ************************************************************************/

#include "osal.h"
#include "pconfig.h"

/*===========================================================================
 * MCU外设配置数组 - 展示多种通信接口
 *===========================================================================*/

/* STM32 MCU 通过 UART 通信 - 载荷控制单元 */
static pconfig_mcu_entry_t mcu_stm32_uart = {
    .name = "stm32_mcu",
    .description = "STM32 MCU for payload control (UART)",
    .enabled = true,

    .config = {
        .name = "stm32_mcu",
        .interface = PDL_MCU_INTERFACE_SERIAL,

        .hw.serial = {
            .device = "/dev/ttyS1",
            .baudrate = 115200,
            .data_bits = 8,
            .stop_bits = 1,
            .parity = PDL_MCU_PARITY_NONE,
            .flow_control = PDL_MCU_FLOW_NONE
        },

        .cmd_timeout_ms = 500,
        .retry_count = 3
        /* 注意：串口通信强制启用 CRC 校验 */
    },

    .reset_gpio = NULL,
    .irq_gpio = NULL
};

/* Power MCU 通过 CAN 总线通信 - 电源管理单元 */
static pconfig_mcu_entry_t mcu_power_can = {
    .name = "power_mcu",
    .description = "Power Management MCU (CAN)",
    .enabled = true,

    .config = {
        .name = "power_mcu",
        .interface = PDL_MCU_INTERFACE_CAN,

        .hw.can = {
            .device = "can0",
            .bitrate = 500000,          /* 500 kbps */
            .rx_timeout = 100,
            .tx_timeout = 100,
            .tx_id = 0x100,             /* 发送 CAN ID */
            .rx_id = 0x101              /* 接收 CAN ID */
        },

        .cmd_timeout_ms = 1000,
        .retry_count = 3
    },

    .reset_gpio = NULL,
    .irq_gpio = NULL
};

/* Sensor MCU 通过 I2C 通信 - 传感器集线器（预留接口） */
static pconfig_mcu_entry_t mcu_sensor_i2c = {
    .name = "sensor_mcu",
    .description = "Sensor Hub MCU (I2C) - Reserved",
    .enabled = false,  /* 预留接口，暂不启用 */

    .config = {
        .name = "sensor_mcu",
        .interface = PDL_MCU_INTERFACE_I2C,
        .hw = {0},      /* I2C 配置待实现 */
        .cmd_timeout_ms = 200,
        .retry_count = 3
    },

    .reset_gpio = NULL,
    .irq_gpio = NULL
};

/* FPGA Control MCU 通过 SPI 通信（预留接口） */
static pconfig_mcu_entry_t mcu_fpga_spi = {
    .name = "fpga_ctrl_mcu",
    .description = "FPGA Control MCU (SPI) - Reserved",
    .enabled = false,  /* 预留接口，暂不启用 */

    .config = {
        .name = "fpga_ctrl_mcu",
        .interface = PDL_MCU_INTERFACE_SPI,
        .hw = {0},      /* SPI 配置待实现 */
        .cmd_timeout_ms = 100,
        .retry_count = 3
    },

    .reset_gpio = NULL,
    .irq_gpio = NULL
};

/* PMC (Payload Management Controller) 通过 CAN 通信 */
static pconfig_mcu_entry_t mcu_pmc_can = {
    .name = "pmc_can",
    .description = "Payload Management Controller (CAN)",
    .enabled = false,  /* 可选设备 */

    .config = {
        .name = "pmc_can",
        .interface = PDL_MCU_INTERFACE_CAN,

        .hw.can = {
            .device = "can0",
            .bitrate = 1000000,         /* 1 Mbps */
            .rx_timeout = 100,
            .tx_timeout = 100,
            .tx_id = 0x200,             /* 发送 CAN ID */
            .rx_id = 0x201              /* 接收 CAN ID */
        },

        .cmd_timeout_ms = 500,
        .retry_count = 3
    },

    .reset_gpio = NULL,
    .irq_gpio = NULL
};

/* PMC 通过 Ethernet (UART bridge) 通信 - 备用通道 */
static pconfig_mcu_entry_t mcu_pmc_ethernet = {
    .name = "pmc_ethernet",
    .description = "PMC via Ethernet (UART bridge)",
    .enabled = false,  /* 备用通道 */

    .config = {
        .name = "pmc_ethernet",
        .interface = PDL_MCU_INTERFACE_SERIAL,

        .hw.serial = {
            .device = "/dev/ttyS3",     /* 串口转以太网桥接 */
            .baudrate = 921600,         /* 高速串口 */
            .data_bits = 8,
            .stop_bits = 1,
            .parity = PDL_MCU_PARITY_NONE,
            .flow_control = PDL_MCU_FLOW_NONE
        },

        .cmd_timeout_ms = 1000,
        .retry_count = 3
    },

    .reset_gpio = NULL,
    .irq_gpio = NULL
};

static pconfig_mcu_entry_t *pconfig_mcu_arr[] = {
    &mcu_stm32_uart,        /* UART 示例 */
    &mcu_power_can,         /* CAN 示例 */
    &mcu_sensor_i2c,        /* I2C 示例（预留） */
    &mcu_fpga_spi,          /* SPI 示例（预留） */
    &mcu_pmc_can,           /* PMC CAN 示例 */
    &mcu_pmc_ethernet,      /* PMC Ethernet 示例 */
    NULL
};

/*===========================================================================
 * BMC外设配置数组 - 展示多种通信方式
 *===========================================================================*/

/* Payload BMC - IPMI over Network + Serial 双通道 */
static pconfig_bmc_entry_t bmc_payload_ipmi = {
    .name = "payload_bmc",
    .description = "Payload BMC with IPMI protocol",
    .enabled = true,

    .config = {
        /* 主通道：IPMI over Network */
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .primary_config = {
            .network = {
                .ip_addr = "192.168.1.100",
                .port = 623,                /* IPMI 标准端口 */
                .username = "admin",
                .password = NULL,
                .timeout_ms = 2000
            }
        },

        /* 备用通道：IPMI over Serial */
        .backup_channel = PDL_BMC_CHANNEL_SERIAL,
        .backup_config = {
            .serial = {
                .device = "/dev/ttyS2",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = PDL_MCU_PARITY_NONE,
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

/* Platform BMC - Redfish over Network (现代接口) */
static pconfig_bmc_entry_t bmc_platform_redfish = {
    .name = "platform_bmc",
    .description = "Platform BMC with Redfish protocol",
    .enabled = false,  /* 可选设备 */

    .config = {
        /* 主通道：Redfish over Network (HTTPS) */
        .primary_channel = PDL_BMC_CHANNEL_NETWORK,
        .primary_config = {
            .network = {
                .ip_addr = "192.168.1.101",
                .port = 443,                /* Redfish 使用 HTTPS */
                .username = "root",
                .password = NULL,
                .timeout_ms = 3000
            }
        },

        /* 备用通道：IPMI over Serial (回退方案) */
        .backup_channel = PDL_BMC_CHANNEL_SERIAL,
        .backup_config = {
            .serial = {
                .device = "/dev/ttyS4",
                .baudrate = 115200,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = PDL_MCU_PARITY_NONE,
                .timeout_ms = 2000
            }
        },

        .auto_switch = true,
        .retry_count = 3,
        .health_check_interval = 10000
    },

    .power_gpio = NULL,
    .reset_gpio = NULL
};

/* Storage BMC - 仅串口通信（简单场景） */
static pconfig_bmc_entry_t bmc_storage_serial = {
    .name = "storage_bmc",
    .description = "Storage BMC (Serial only)",
    .enabled = false,

    .config = {
        /* 主通道：Serial only */
        .primary_channel = PDL_BMC_CHANNEL_SERIAL,
        .primary_config = {
            .serial = {
                .device = "/dev/ttyS5",
                .baudrate = 57600,
                .data_bits = 8,
                .stop_bits = 1,
                .parity = PDL_MCU_PARITY_NONE,
                .timeout_ms = 1500
            }
        },

        /* 无备用通道 */
        .backup_channel = PDL_BMC_CHANNEL_NETWORK,
        .backup_config = {0},

        .auto_switch = false,
        .retry_count = 5,
        .health_check_interval = 5000
    },

    .power_gpio = NULL,
    .reset_gpio = NULL
};

static pconfig_bmc_entry_t *pconfig_bmc_arr[] = {
    &bmc_payload_ipmi,      /* IPMI Network + Serial */
    &bmc_platform_redfish,  /* Redfish Network */
    &bmc_storage_serial,    /* Serial only */
    NULL
};

/*===========================================================================
 * FPGA外设配置数组
 *===========================================================================*/

/* FPGA 逻辑控制设备 */
static pconfig_fpga_cfg_t fpga_logic = {
    .name = "fpga_logic",
    .description = "FPGA for custom logic",
    .enabled = false,  /* 可选设备 */

    .device = "/dev/fpga0",
    .cmd_timeout_ms = 1000,
    .retry_count = 3
};

static pconfig_fpga_cfg_t *pconfig_fpga_arr[] = {
    &fpga_logic,
    NULL
};

/*===========================================================================
 * Switch外设配置数组
 *===========================================================================*/

/* 以太网交换机 */
static pconfig_switch_cfg_t switch_ethernet = {
    .name = "eth_switch",
    .description = "Ethernet switch for network routing",
    .enabled = false,  /* 可选设备 */

    .device = "/dev/switch0",
    .cmd_timeout_ms = 500,
    .retry_count = 3
};

static pconfig_switch_cfg_t *pconfig_switch_arr[] = {
    &switch_ethernet,
    NULL
};

/*===========================================================================
 * 平台配置注册
 *===========================================================================*/

/* 辅助宏：计算指针数组中有效元素数量（排除NULL结尾） */
#define ARRAY_COUNT(arr) ((OSAL_sizeof(arr) / OSAL_sizeof(arr[0])) - 1)

const pconfig_platform_config_t pconfig_h200_100p_am625 = {
    .platform_name = "ti/am6254",
    .chip_name = "am6254",
    .project_name = "H200_100P_AM625",
    .product_name = "h200_100p_am625",

    /* MCU 外设：
     * - mcu_stm32_uart (UART)
     * - mcu_power_can (CAN)
     * - mcu_sensor_i2c (I2C - 预留)
     * - mcu_fpga_spi (SPI - 预留)
     * - mcu_pmc_can (PMC via CAN)
     * - mcu_pmc_ethernet (PMC via Ethernet)
     */
    .mcu_count = 6,
    .mcu_arr = pconfig_mcu_arr,

    /* BMC 外设：
     * - bmc_payload_ipmi (IPMI Network + Serial)
     * - bmc_platform_redfish (Redfish Network)
     * - bmc_storage_serial (Serial only)
     */
    .bmc_count = 3,
    .bmc_arr = pconfig_bmc_arr,

    /* FPGA 外设 */
    .fpga_count = 1,
    .fpga_arr = pconfig_fpga_arr,

    /* Switch 外设 */
    .switch_count = 1,
    .switch_arr = pconfig_switch_arr
};

/*===========================================================================
 * 配置说明和使用指南
 *===========================================================================*/

/*
 * ========================================================================
 * 通信接口总结
 * ========================================================================
 *
 * 1. MCU 设备支持的接口类型：
 *    ┌──────────┬────────────────────────────────────────────┐
 *    │ 接口类型 │ 说明                                       │
 *    ├──────────┼────────────────────────────────────────────┤
 *    │ CAN      │ CAN 总线，高可靠性，常用于车载/航天       │
 *    │ UART     │ 串口，通用接口，强制 CRC 校验             │
 *    │ I2C      │ I2C 总线（预留），适用于传感器连接        │
 *    │ SPI      │ SPI 总线（预留），适用于高速外设          │
 *    └──────────┴────────────────────────────────────────────┘
 *
 * 2. BMC 设备支持的通信方式：
 *    ┌──────────┬────────────────────────────────────────────┐
 *    │ 通道类型 │ 协议                                       │
 *    ├──────────┼────────────────────────────────────────────┤
 *    │ Network  │ IPMI over LAN (端口 623)                  │
 *    │          │ Redfish over HTTPS (端口 443)             │
 *    │ Serial   │ IPMI over Serial                          │
 *    └──────────┴────────────────────────────────────────────┘
 *
 * 3. 设备映射关系：
 *    - 传感器设备: 通过 MCU 的 I2C/SPI 接口连接
 *    - PMC 设备: 通过 MCU 的 CAN/Ethernet(UART bridge) 接口连接
 *    - BMC 设备: 直接使用网络或串口连接，支持 IPMI/Redfish 协议
 *    - FPGA/Switch: 专用设备节点
 *
 * ========================================================================
 * 示例外设列表
 * ========================================================================
 *
 * MCU 外设 (6 个)：
 *   [✓] mcu_stm32_uart      - STM32 MCU via UART (载荷控制)
 *   [✓] mcu_power_can       - Power MCU via CAN (电源管理)
 *   [ ] mcu_sensor_i2c      - Sensor MCU via I2C (传感器集线器，预留)
 *   [ ] mcu_fpga_spi        - FPGA Control via SPI (FPGA 控制，预留)
 *   [ ] mcu_pmc_can         - PMC via CAN (载荷管理控制器)
 *   [ ] mcu_pmc_ethernet    - PMC via Ethernet bridge (PMC 备用通道)
 *
 * BMC 外设 (3 个)：
 *   [✓] bmc_payload_ipmi    - Payload BMC via IPMI (Network + Serial)
 *   [ ] bmc_platform_redfish- Platform BMC via Redfish (Network + Serial)
 *   [ ] bmc_storage_serial  - Storage BMC via Serial only
 *
 * FPGA 外设 (1 个)：
 *   [ ] fpga_logic          - FPGA 逻辑控制
 *
 * Switch 外设 (1 个)：
 *   [ ] switch_ethernet     - 以太网交换机
 *
 * 图例：[✓] 启用  [ ] 禁用
 *
 * ========================================================================
 * 配置选择建议
 * ========================================================================
 *
 * 1. 关键设备配置：
 *    - 使用双通道配置（主通道 + 备用通道）
 *    - 启用 auto_switch 实现自动故障切换
 *    - 设置合理的 retry_count 和 timeout
 *
 * 2. 通信接口选择：
 *    ┌────────────┬──────────┬──────────────────────┐
 *    │ 场景       │ 推荐接口 │ 理由                 │
 *    ├────────────┼──────────┼──────────────────────┤
 *    │ 实时控制   │ CAN      │ 确定性延迟，高可靠   │
 *    │ 大数据传输 │ Ethernet │ 高带宽，灵活性好     │
 *    │ 简单通信   │ UART     │ 通用，易于调试       │
 *    │ 传感器     │ I2C/SPI  │ 低功耗，多设备共享   │
 *    └────────────┴──────────┴──────────────────────┘
 *
 * 3. BMC 协议选择：
 *    - IPMI: 成熟稳定，广泛支持，适合传统系统
 *    - Redfish: 现代 RESTful API，适合云原生系统
 *
 * ========================================================================
 * 启用/禁用外设
 * ========================================================================
 *
 * 根据实际硬件配置修改 .enabled 字段：
 *
 *   .enabled = true   - 外设存在并需要初始化
 *   .enabled = false  - 外设不存在或不需要使用
 *
 * 示例：启用 PMC CAN 通信
 *   static pconfig_mcu_entry_t mcu_pmc_can = {
 *       .enabled = true,  // 修改这里
 *       ...
 *   };
 *
 * ========================================================================
 * 添加新外设
 * ========================================================================
 *
 * 步骤 1: 定义外设配置结构
 *   static pconfig_mcu_entry_t mcu_new_device = {
 *       .name = "new_device",
 *       .description = "New device description",
 *       .enabled = true,
 *       .config = { ... },
 *       ...
 *   };
 *
 * 步骤 2: 添加到外设数组
 *   static pconfig_mcu_entry_t *pconfig_mcu_arr[] = {
 *       &mcu_stm32_uart,
 *       &mcu_new_device,  // 添加这里
 *       NULL
 *   };
 *
 * 步骤 3: 更新外设计数
 *   .mcu_count = 7,  // 原来是 6，增加 1
 *
 * ========================================================================
 * 接口参数配置参考
 * ========================================================================
 *
 * CAN 总线配置：
 *   .bitrate      - 常用值: 125k, 250k, 500k, 1000k (bps)
 *   .tx_id/rx_id  - CAN ID 分配，避免冲突
 *
 * UART 串口配置：
 *   .baudrate     - 常用值: 9600, 57600, 115200, 921600
 *   .data_bits    - 通常为 8
 *   .stop_bits    - 通常为 1
 *   .parity       - 通常为 NONE
 *
 * 网络配置：
 *   .port         - IPMI: 623, Redfish: 443
 *   .timeout_ms   - 建议 2000-5000ms
 *
 * ========================================================================
 * 故障排查
 * ========================================================================
 *
 * 1. 设备初始化失败：
 *    - 检查 .enabled 是否为 true
 *    - 检查设备节点路径是否正确 (/dev/ttyS1, can0 等)
 *    - 检查设备驱动是否加载 (lsmod | grep can)
 *
 * 2. 通信超时：
 *    - 增加 .cmd_timeout_ms 值
 *    - 检查物理连接和硬件状态
 *    - 检查波特率/CAN bitrate 是否匹配
 *
 * 3. CAN ID 冲突：
 *    - 确保每个 CAN 设备的 tx_id/rx_id 唯一
 *    - 使用 candump 工具查看总线流量
 *
 * 4. BMC 通道切换失败：
 *    - 检查 .auto_switch 是否启用
 *    - 检查备用通道配置是否正确
 *    - 查看健康检查日志
 *
 * ========================================================================
 */
