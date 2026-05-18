/************************************************************************
 * 演示平台 - 演示项目载荷板 V1.0配置
 *
 * 平台：演示平台 (用于模拟测试和示例演示)
 * 产品：演示项目算存载荷转接板（2P算力）
 * 版本：V1.0（首批生产版本）
 *
 * 硬件变更（相对于base）：
 * - 增加第二个MCU（用于冗余控制）
 * - 增加陀螺仪和加速度计传感器
 * - 卫星接口增加冗余CAN通道
 ************************************************************************/

#include "pcl.h"

/*===========================================================================
 * V1.0特有GPIO定义
 *===========================================================================*/

static pcl_gpio_config_t gpio_mcu2_reset = {
    .gpio_num = 60,               /* GPIO1_28 */
    .pin_mux = 0x07,
    .active_low = true,
    .pull_up = true,
    .pull_down = false
};

static pcl_gpio_config_t gpio_imu_power = {
    .gpio_num = 61,               /* GPIO1_29 */
    .pin_mux = 0x07,
    .active_low = false,
    .pull_up = false,
    .pull_down = true
};

/*===========================================================================
 * MCU外设配置（V1.0新增）
 *===========================================================================*/

/**
 * 第二个MCU（冗余控制）
 * - 通信接口：CAN1
 * - 功能：冗余控制、故障检测
 */
static pcl_mcu_cfg_t mcu_backup = {
    .name = "backup_mcu",
    .description = "Backup MCU for redundancy",
    .enabled = true,

    /* 通信接口：CAN */
    .interface_type = PCL_HW_INTERFACE_CAN,
    .interface_cfg.can = {
        .device = "can1",
        .bitrate = 500000,
        .tx_id = 0x400,
        .rx_id = 0x500
    },

    .cmd_timeout_ms = 500,
    .retry_count = 3,
    .enable_crc = true,

    .reset_gpio = &gpio_mcu2_reset,
    .irq_gpio = NULL
};

static pcl_mcu_cfg_t *mcu_list_v1[] = {
    &mcu_backup
    /* base的MCU在运行时合并 */
};

/*===========================================================================
 * 卫星平台接口配置（V1.0增强）
 *===========================================================================*/

/**
 * 卫星平台冗余接口
 * - 通信接口：CAN2（备份通道）
 */
static pcl_satellite_cfg_t satellite_backup = {
    .name = "satellite_backup",
    .description = "Satellite platform backup CAN interface",
    .enabled = true,

    .interface_type = PCL_HW_INTERFACE_CAN,
    .interface_cfg.can = {
        .device = "can2",
        .bitrate = 500000,
        .tx_id = 0x301,
        .rx_id = 0x101
    },

    .cmd_timeout_ms = 1000,
    .retry_count = 3,
    .enable_telemetry = true
};

static pcl_satellite_cfg_t *satellite_list_v1[] = {
    &satellite_backup
};

/*===========================================================================
 * 传感器外设配置（V1.0新增）
 *===========================================================================*/

/**
 * 陀螺仪传感器
 * - 通信接口：I2C1
 * - 型号：MPU6050（示例）
 */
static pcl_sensor_cfg_t sensor_gyro = {
    .name = "board_gyro",
    .description = "Gyroscope sensor (MPU6050)",
    .type = SENSOR_TYPE_GYROSCOPE,
    .enabled = true,

    .interface_type = PCL_HW_INTERFACE_I2C,
    .interface_cfg.i2c = {
        .device = "/dev/i2c-1",
        .slave_addr = 0x68,
        .speed_hz = 400000
    },

    .sample_rate = 100,           /* 100Hz */
    .resolution = 16,             /* 16位 */

    .power_gpio = &gpio_imu_power,
    .irq_gpio = NULL
};

/**
 * 加速度计传感器
 * - 通信接口：I2C1（与陀螺仪共用总线）
 * - 型号：MPU6050内置加速度计
 */
static pcl_sensor_cfg_t sensor_accel = {
    .name = "board_accel",
    .description = "Accelerometer sensor (MPU6050)",
    .type = SENSOR_TYPE_ACCELEROMETER,
    .enabled = true,

    .interface_type = PCL_HW_INTERFACE_I2C,
    .interface_cfg.i2c = {
        .device = "/dev/i2c-1",
        .slave_addr = 0x68,       /* 与陀螺仪同地址 */
        .speed_hz = 400000
    },

    .sample_rate = 100,
    .resolution = 16,

    .power_gpio = &gpio_imu_power,
    .irq_gpio = NULL
};

static pcl_sensor_cfg_t *sensor_list_v1[] = {
    &sensor_gyro,
    &sensor_accel
};

/*===========================================================================
 * 电源域配置（V1.0新增）
 *===========================================================================*/

static pcl_power_domain_t power_imu = {
    .name = "imu_power",
    .enable_gpio = &gpio_imu_power,
    .voltage_mv = 3300,
    .current_ma = 200,
    .startup_delay_ms = 100
};

static pcl_power_domain_t *power_domain_list_v1[] = {
    &power_imu
};

/*===========================================================================
 * APP配置（V1.0）
 *===========================================================================*/

/* CAN网关APP配置（V1.0增加备份卫星接口） */
static pcl_app_device_mapping_t can_gateway_devices_v1[] = {
    {
        .function = "satellite_comm",
        .device_type = PCL_DEV_SATELLITE,
        .device_id = 0,           /* 主卫星接口 */
        .required = true
    },
    {
        .function = "satellite_comm_backup",
        .device_type = PCL_DEV_SATELLITE,
        .device_id = 1,           /* 备份卫星接口（V1新增） */
        .required = false
    },
    {
        .function = "aux_control",
        .device_type = PCL_DEV_MCU,
        .device_id = 0,
        .required = false
    },
    {
        .function = "aux_control_backup",
        .device_type = PCL_DEV_MCU,
        .device_id = 1,           /* 备份MCU（V1新增） */
        .required = false
    }
};

static pcl_app_config_t app_can_gateway_v1 = {
    .app_name = "can_gateway",
    .description = "CAN Gateway Application V1.0 - with redundancy",
    .device_mappings = can_gateway_devices_v1,
    .mapping_count = sizeof(can_gateway_devices_v1) / sizeof(can_gateway_devices_v1[0]),
    .params = {
        .heartbeat_interval_ms = 5000,
        .cmd_timeout_ms = 1000,
        .retry_count = 3,
        .queue_depth = 10,
        .failover_threshold = 3   /* V1支持故障切换 */
    }
};

/* APP配置列表 */
static pcl_app_config_t *app_list_v1[] = {
    &app_can_gateway_v1
};

/*===========================================================================
 * 板级配置（导出）
 *===========================================================================*/

const pcl_board_config_t pcl_demo_v1 = {
    .platform = "vendor_demo/platform_demo/project_demo",
    .product = "DEMO_PROJECT",
    .version = "v1.0",
    .description = "Demo Project Payload Adapter Board V1.0 - Demo/Simulation Platform",

    /* V1.0特有外设（base外设在运行时合并） */
    .mcus = mcu_list_v1,
    .mcu_count = sizeof(mcu_list_v1) / sizeof(mcu_list_v1[0]),

    .bmcs = NULL,
    .bmc_count = 0,

    .satellites = satellite_list_v1,
    .satellite_count = sizeof(satellite_list_v1) / sizeof(satellite_list_v1[0]),

    .sensors = sensor_list_v1,
    .sensor_count = sizeof(sensor_list_v1) / sizeof(sensor_list_v1[0]),

    .storages = NULL,
    .storage_count = 0,

    .power_domains = power_domain_list_v1,
    .power_domain_count = sizeof(power_domain_list_v1) / sizeof(power_domain_list_v1[0]),

    /* APP配置 */
    .apps = app_list_v1,
    .app_count = sizeof(app_list_v1) / sizeof(app_list_v1[0]),

    .private_data = NULL
};
