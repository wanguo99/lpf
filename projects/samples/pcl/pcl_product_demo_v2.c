/************************************************************************
 * 演示平台 - 演示项目载荷板 V2.0配置
 *
 * 平台：演示平台 (用于模拟测试和示例演示)
 * 产品：演示项目算存载荷转接板（2P算力）
 * 版本：V2.0（改进版本）
 *
 * 硬件变更（相对于V1.0）：
 * - 卫星接口升级为MIL-STD-1553B（替代CAN）
 * - 增加GPS传感器
 * - 增加NVMe存储设备
 * - BMC升级为2.5G以太网
 ************************************************************************/

#include "pcl.h"

/*===========================================================================
 * V2.0特有GPIO定义
 *===========================================================================*/

static pcl_gpio_config_t gpio_1553b_reset __attribute__((unused)) = {
    .gpio_num = 70,               /* GPIO2_6 */
    .pin_mux = 0x07,
    .active_low = true,
    .pull_up = true,
    .pull_down = false
};

static pcl_gpio_config_t gpio_gps_power = {
    .gpio_num = 71,               /* GPIO2_7 */
    .pin_mux = 0x07,
    .active_low = false,
    .pull_up = false,
    .pull_down = true
};

static pcl_gpio_config_t gpio_nvme_power = {
    .gpio_num = 72,               /* GPIO2_8 */
    .pin_mux = 0x07,
    .active_low = false,
    .pull_up = false,
    .pull_down = true
};

/*===========================================================================
 * 卫星平台接口配置（V2.0升级）
 *===========================================================================*/

/**
 * 卫星平台1553B接口
 * - 通信接口：MIL-STD-1553B（替代CAN）
 * - 功能：高可靠性航天总线通信
 */
static pcl_satellite_cfg_t satellite_1553b = {
    .name = "satellite_1553b",
    .description = "Satellite platform 1553B interface",
    .enabled = true,

    /* 通信接口：1553B */
    .interface_type = PCL_HW_INTERFACE_1553B,
    .interface_cfg.mil1553b = {
        .device = "/dev/mil1553b0",
        .rt_address = 5           /* RT地址：5 */
    },

    .cmd_timeout_ms = 2000,
    .retry_count = 3,
    .enable_telemetry = true
};

static pcl_satellite_cfg_t *satellite_list_v2[] = {
    &satellite_1553b
};

/*===========================================================================
 * BMC外设配置（V2.0升级）
 *===========================================================================*/

/**
 * 载荷BMC外设（升级版）
 * - 主通道：2.5G以太网
 * - 备份通道：串口
 */
static pcl_bmc_cfg_t bmc_payload_v2 = {
    .name = "payload_bmc_v2",
    .description = "Payload BMC with 2.5G Ethernet",
    .enabled = true,

    /* 主通道：2.5G以太网 */
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

    /* 备份通道：串口 */
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

static pcl_bmc_cfg_t *bmc_list_v2[] = {
    &bmc_payload_v2
};

/*===========================================================================
 * 传感器外设配置（V2.0新增）
 *===========================================================================*/

/**
 * GPS传感器
 * - 通信接口：UART3
 * - 功能：位置和时间同步
 */
static pcl_sensor_cfg_t sensor_gps = {
    .name = "board_gps",
    .description = "GPS sensor for position and time sync",
    .type = SENSOR_TYPE_GPS,
    .enabled = true,

    /* 通信接口：UART */
    .interface_type = PCL_HW_INTERFACE_UART,
    .interface_cfg.uart = {
        .device = "/dev/ttyS3",
        .baudrate = 9600,         /* GPS标准波特率 */
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 'N'
    },

    .sample_rate = 1,             /* 1Hz */
    .resolution = 0,              /* N/A for GPS */

    .power_gpio = &gpio_gps_power,
    .irq_gpio = NULL
};

static pcl_sensor_cfg_t *sensor_list_v2[] = {
    &sensor_gps
};

/*===========================================================================
 * 存储设备配置（V2.0新增）
 *===========================================================================*/

/**
 * NVMe SSD存储设备
 * - 容量：256GB
 * - 功能：高速数据存储
 */
static pcl_storage_cfg_t storage_nvme = {
    .name = "nvme_storage",
    .description = "256GB NVMe SSD for high-speed data storage",
    .type = STORAGE_TYPE_NVME,
    .enabled = true,

    .device_path = "/dev/nvme0n1",

    .capacity_mb = 262144,        /* 256GB */
    .block_size = 4096,           /* 4KB */

    .power_gpio = &gpio_nvme_power
};

static pcl_storage_cfg_t *storage_list_v2[] = {
    &storage_nvme
};

/*===========================================================================
 * 电源域配置（V2.0新增）
 *===========================================================================*/

static pcl_power_domain_t power_gps = {
    .name = "gps_power",
    .enable_gpio = &gpio_gps_power,
    .voltage_mv = 3300,
    .current_ma = 100,
    .startup_delay_ms = 200       /* GPS需要较长启动时间 */
};

static pcl_power_domain_t power_nvme = {
    .name = "nvme_power",
    .enable_gpio = &gpio_nvme_power,
    .voltage_mv = 3300,
    .current_ma = 2000,           /* NVMe功耗较大 */
    .startup_delay_ms = 500
};

static pcl_power_domain_t *power_domain_list_v2[] = {
    &power_gps,
    &power_nvme
};

/*===========================================================================
 * APP配置（V2.0）
 *===========================================================================*/

/* 协议转换器APP配置（V2.0使用1553B接口） */
static pcl_app_device_mapping_t protocol_converter_devices_v2[] = {
    {
        .function = "satellite_comm",
        .device_type = PCL_DEV_SATELLITE,
        .device_id = 0            /* V2使用1553B接口 */
    },
    {
        .function = "payload_comm",
        .device_type = PCL_DEV_BMC,
        .device_id = 0            /* V2使用升级的BMC */
    },
    {
        .function = "position_sensor",
        .device_type = PCL_DEV_SENSOR,
        .device_id = 0            /* V2新增GPS传感器 */
    }
};

static pcl_app_config_t app_protocol_converter_v2 = {
    .app_name = "protocol_converter",
    .description = "Protocol Converter Application V2.0 - with 1553B and GPS",
    .device_mappings = protocol_converter_devices_v2,
    .mapping_count = sizeof(protocol_converter_devices_v2) / sizeof(protocol_converter_devices_v2[0]),
    .params = {
        .heartbeat_interval_ms = 0,
        .cmd_timeout_ms = 2000,
        .retry_count = 3,
        .queue_depth = 0,
        .failover_threshold = 5
    }
};

/* APP配置列表 */
static pcl_app_config_t *app_list_v2[] = {
    &app_protocol_converter_v2
};

/*===========================================================================
 * 板级配置（导出）
 *===========================================================================*/

const pcl_board_config_t pcl_demo_v2 = {
    .platform = "vendor_demo/platform_demo/project_demo",
    .product = "DEMO_PROJECT",
    .version = "v2.0",
    .description = "Demo Project Payload Adapter Board V2.0 - Demo/Simulation Platform",

    /* V2.0特有外设 */
    .mcus = NULL,
    .mcu_count = 0,

    .bmcs = bmc_list_v2,
    .bmc_count = sizeof(bmc_list_v2) / sizeof(bmc_list_v2[0]),

    .satellites = satellite_list_v2,
    .satellite_count = sizeof(satellite_list_v2) / sizeof(satellite_list_v2[0]),

    .sensors = sensor_list_v2,
    .sensor_count = sizeof(sensor_list_v2) / sizeof(sensor_list_v2[0]),

    .storages = storage_list_v2,
    .storage_count = sizeof(storage_list_v2) / sizeof(storage_list_v2[0]),

    .power_domains = power_domain_list_v2,
    .power_domain_count = sizeof(power_domain_list_v2) / sizeof(power_domain_list_v2[0]),

    /* APP配置 */
    .apps = app_list_v2,
    .app_count = sizeof(app_list_v2) / sizeof(app_list_v2[0]),

    .private_data = NULL
};
