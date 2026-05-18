/************************************************************************
 * 演示平台 - 演示项目载荷板基础配置
 *
 * 平台：演示平台 (用于模拟测试和示例演示)
 * 产品：演示项目算存载荷转接板（2P算力）
 * 版本：Base（基础配置，所有版本共享）
 *
 * 配置理念：以外设为单位进行配置
 ************************************************************************/

#include "pcl.h"

/*===========================================================================
 * GPIO定义
 *===========================================================================*/

/* MCU控制GPIO */
static pcl_gpio_config_t gpio_mcu_reset = {
    .gpio_num = 42,               /* GPIO1_10 */
    .pin_mux = 0x07,              /* Mode 7: GPIO */
    .active_low = true,           /* 低电平复位 */
    .pull_up = true,
    .pull_down = false
};

static pcl_gpio_config_t gpio_mcu_irq = {
    .gpio_num = 43,               /* GPIO1_11 */
    .pin_mux = 0x07,
    .active_low = true,
    .pull_up = true,
    .pull_down = false
};

/* BMC控制GPIO */
static pcl_gpio_config_t gpio_bmc_power = {
    .gpio_num = 50,               /* GPIO1_18 */
    .pin_mux = 0x07,
    .active_low = false,
    .pull_up = false,
    .pull_down = true
};

static pcl_gpio_config_t gpio_bmc_reset = {
    .gpio_num = 51,               /* GPIO1_19 */
    .pin_mux = 0x07,
    .active_low = true,
    .pull_up = true,
    .pull_down = false
};

/* 传感器控制GPIO */
static pcl_gpio_config_t gpio_sensor_power = {
    .gpio_num = 52,               /* GPIO1_20 */
    .pin_mux = 0x07,
    .active_low = false,
    .pull_up = false,
    .pull_down = true
};

/*===========================================================================
 * MCU外设配置
 *===========================================================================*/

/**
 * STM32 MCU外设
 * - 通信接口：UART1
 * - 功能：辅助控制、GPIO扩展、ADC采集
 */
static pcl_mcu_cfg_t mcu_stm32 = {
    /* 外设基本信息 */
    .name = "stm32_mcu",
    .description = "STM32 MCU for auxiliary control",
    .enabled = true,

    /* 通信接口：UART */
    .interface_type = PCL_HW_INTERFACE_UART,
    .interface_cfg.uart = {
        .device = "/dev/ttyS1",
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 'N'
    },

    /* MCU特定配置 */
    .cmd_timeout_ms = 500,
    .retry_count = 3,
    .enable_crc = true,

    /* GPIO控制 */
    .reset_gpio = &gpio_mcu_reset,
    .irq_gpio = &gpio_mcu_irq
};

static pcl_mcu_cfg_t *mcu_list[] = {
    &mcu_stm32
};

/*===========================================================================
 * BMC外设配置
 *===========================================================================*/

/**
 * 载荷BMC外设
 * - 主通道：以太网（IPMI over LAN）
 * - 备份通道：串口（IPMI over Serial）
 * - 功能：载荷电源管理、状态监控、传感器读取
 */
static pcl_bmc_cfg_t bmc_payload = {
    /* 外设基本信息 */
    .name = "payload_bmc",
    .description = "Payload BMC for power and thermal management",
    .enabled = true,

    /* 主通道：IPMI over LAN */
    .primary_channel = {
        .protocol = PCL_BMC_PROTOCOL_IPMI,
        .cfg.ipmi_lan = {
            .interface = "eth0",
            .ip_addr = "192.168.1.100",
            .port = 623,             /* IPMI端口 */
            .username = "admin",
            .password = NULL
        }
    },

    /* 备份通道：IPMI over Serial */
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

    /* BMC特定配置 */
    .cmd_timeout_ms = 2000,
    .retry_count = 3,
    .failover_threshold = 5,      /* 连续5次失败后切换通道 */

    /* GPIO控制 */
    .power_gpio = &gpio_bmc_power,
    .reset_gpio = &gpio_bmc_reset
};

static pcl_bmc_cfg_t *bmc_list[] = {
    &bmc_payload
};

/*===========================================================================
 * 卫星平台接口配置
 *===========================================================================*/

/**
 * 卫星平台接口
 * - 通信接口：CAN0
 * - 功能：接收卫星平台命令，上报载荷状态
 */
static pcl_satellite_cfg_t satellite_platform = {
    /* 外设基本信息 */
    .name = "satellite_platform",
    .description = "Satellite platform CAN interface",
    .enabled = true,

    /* 通信接口：CAN */
    .interface_type = PCL_HW_INTERFACE_CAN,
    .interface_cfg.can = {
        .device = "can0",
        .bitrate = 500000,        /* 500Kbps */
        .tx_id = 0x300,           /* 转接板发送ID */
        .rx_id = 0x100            /* 卫星平台ID */
    },

    /* 卫星平台特定配置 */
    .cmd_timeout_ms = 1000,
    .retry_count = 3,
    .enable_telemetry = true,

    .power_gpio = NULL,
    .reset_gpio = NULL
};

static pcl_satellite_cfg_t *satellite_list[] = {
    &satellite_platform
};

/*===========================================================================
 * 传感器外设配置
 *===========================================================================*/

/**
 * 板载温度传感器
 * - 通信接口：I2C1
 * - 型号：TMP75（示例）
 * - 功能：监控板载温度
 */
static pcl_sensor_cfg_t sensor_board_temp = {
    /* 外设基本信息 */
    .name = "board_temp",
    .description = "Board temperature sensor (TMP75)",
    .type = SENSOR_TYPE_TEMPERATURE,
    .enabled = true,

    /* 通信接口：I2C */
    .interface_type = PCL_HW_INTERFACE_I2C,
    .interface_cfg.i2c = {
        .device = "/dev/i2c-1",
        .slave_addr = 0x48,       /* TMP75地址 */
        .speed_hz = 400000        /* 400kHz */
    },

    /* 传感器特定配置 */
    .sample_rate = 1,             /* 1Hz */
    .resolution = 12,             /* 12位 */

    /* GPIO控制 */
    .power_gpio = &gpio_sensor_power,
    .irq_gpio = NULL
};

static pcl_sensor_cfg_t *sensor_list[] = {
    &sensor_board_temp
};

/*===========================================================================
 * 存储设备配置
 *===========================================================================*/

/**
 * eMMC存储设备
 * - 容量：16GB
 * - 功能：系统启动、数据存储
 */
static pcl_storage_cfg_t storage_emmc = {
    /* 外设基本信息 */
    .name = "emmc_storage",
    .description = "16GB eMMC for system and data",
    .type = STORAGE_TYPE_EMMC,
    .enabled = true,

    /* 设备路径 */
    .device_path = "/dev/mmcblk0",

    /* 存储特定配置 */
    .capacity_mb = 16384,         /* 16GB */
    .block_size = 512,

    /* GPIO控制 */
    .power_gpio = NULL
};

static pcl_storage_cfg_t *storage_list[] = {
    &storage_emmc
};

/*===========================================================================
 * 电源域配置
 *===========================================================================*/

static pcl_power_domain_t power_payload = {
    .name = "payload_power",
    .enable_gpio = &gpio_bmc_power,
    .voltage_mv = 12000,          /* 12V */
    .current_ma = 5000,           /* 5A */
    .startup_delay_ms = 100
};

static pcl_power_domain_t power_sensor = {
    .name = "sensor_power",
    .enable_gpio = &gpio_sensor_power,
    .voltage_mv = 3300,           /* 3.3V */
    .current_ma = 500,            /* 500mA */
    .startup_delay_ms = 50
};

static pcl_power_domain_t *power_domain_list[] = {
    &power_payload,
    &power_sensor
};

/*===========================================================================
 * APP配置
 *===========================================================================*/

/* CAN网关APP配置 */
static pcl_app_device_mapping_t can_gateway_devices[] = {
    {
        .function = "satellite_comm",
        .device_type = PCL_DEV_SATELLITE,
        .device_id = 0,           /* 使用第0个卫星接口 */
        .required = true
    },
    {
        .function = "aux_control",
        .device_type = PCL_DEV_MCU,
        .device_id = 0,           /* 使用第0个MCU */
        .required = false
    }
};

static pcl_app_config_t app_can_gateway = {
    .app_name = "can_gateway",
    .description = "CAN Gateway Application",
    .device_mappings = can_gateway_devices,
    .mapping_count = sizeof(can_gateway_devices) / sizeof(can_gateway_devices[0]),
    .params = {
        .heartbeat_interval_ms = 5000,
        .cmd_timeout_ms = 1000,
        .retry_count = 3,
        .queue_depth = 10,
        .failover_threshold = 0
    }
};

/* 协议转换器APP配置 */
static pcl_app_device_mapping_t protocol_converter_devices[] = {
    {
        .function = "satellite_comm",
        .device_type = PCL_DEV_SATELLITE,
        .device_id = 0
    },
    {
        .function = "payload_comm",
        .device_type = PCL_DEV_BMC,
        .device_id = 0            /* 使用第0个BMC */
    }
};

static pcl_app_config_t app_protocol_converter = {
    .app_name = "protocol_converter",
    .description = "Protocol Converter Application",
    .device_mappings = protocol_converter_devices,
    .mapping_count = sizeof(protocol_converter_devices) / sizeof(protocol_converter_devices[0]),
    .params = {
        .heartbeat_interval_ms = 0,
        .cmd_timeout_ms = 2000,
        .retry_count = 3,
        .queue_depth = 0,
        .failover_threshold = 5
    }
};

/* APP配置列表 */
static pcl_app_config_t *app_list[] = {
    &app_can_gateway,
    &app_protocol_converter
};

/*===========================================================================
 * 板级配置（导出）
 *===========================================================================*/

const pcl_board_config_t pcl_demo_base = {
    .platform = "vendor_demo/platform_demo/project_demo",
    .product = "DEMO_PROJECT",
    .version = "base",
    .description = "Demo Project Payload Adapter Board (2P Computing Power) - Demo/Simulation Platform",

    /* 外设配置（以外设为单位） */
    .mcus = mcu_list,
    .mcu_count = sizeof(mcu_list) / sizeof(mcu_list[0]),

    .bmcs = bmc_list,
    .bmc_count = sizeof(bmc_list) / sizeof(bmc_list[0]),

    .satellites = satellite_list,
    .satellite_count = sizeof(satellite_list) / sizeof(satellite_list[0]),

    .sensors = sensor_list,
    .sensor_count = sizeof(sensor_list) / sizeof(sensor_list[0]),

    .storages = storage_list,
    .storage_count = sizeof(storage_list) / sizeof(storage_list[0]),

    .power_domains = power_domain_list,
    .power_domain_count = sizeof(power_domain_list) / sizeof(power_domain_list[0]),

    /* APP配置 */
    .apps = app_list,
    .app_count = sizeof(app_list) / sizeof(app_list[0]),

    .private_data = NULL
};
