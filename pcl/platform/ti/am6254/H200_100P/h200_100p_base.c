/************************************************************************
 * TI AM625平台 - H200-100P载荷板基础配置
 *
 * 平台：TI AM625 (Sitara ARM Cortex-A53)
 * 产品：H200-100P算存载荷转接板（100P算力）
 * 版本：Base（基础配置，所有版本共享）
 *
 * 配置理念：以外设为单位进行配置
 *
 * 更新说明：配置结构已重构以完全匹配PDL层需求
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
 * MCU外设配置（匹配PDL层mcu_config_t）
 *===========================================================================*/

/**
 * STM32 MCU外设
 * - 通信接口：UART1
 * - 功能：辅助控制、GPIO扩展、ADC采集
 */
static pcl_mcu_cfg_t mcu_stm32 = {
    /* PCL层扩展字段 */
    .pcl_name = "stm32_mcu",
    .description = "STM32 MCU for auxiliary control",
    .enabled = true,

    /* PDL层配置字段 */
    .name = "stm32_mcu",          /* 传递给PDL的名称 */
    .interface = PCL_MCU_INTERFACE_SERIAL,

    /* 串口配置 */
    .serial = {
        .device = "/dev/ttyS1",
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 'N'
    },

    /* 通用配置 */
    .cmd_timeout_ms = 500,
    .retry_count = 3,
    .enable_crc = true,

    /* GPIO控制 */
    .reset_gpio = &gpio_mcu_reset,
    .irq_gpio = &gpio_mcu_irq
};

static pcl_mcu_cfg_t *mcu_list[] = {
    &mcu_stm32,
    NULL
};

/*===========================================================================
 * BMC外设配置（匹配PDL层bmc_config_t）
 *===========================================================================*/

/**
 * 载荷BMC外设
 * - 主通道：以太网（IPMI over LAN）
 * - 备份通道：串口（IPMI over Serial）
 * - 功能：载荷电源管理、状态监控、传感器读取
 */
static pcl_bmc_cfg_t bmc_payload = {
    /* PCL层扩展字段 */
    .name = "payload_bmc",
    .description = "Payload BMC for power and thermal management",
    .enabled = true,

    /* 网络通道配置（PDL层） */
    .network = {
        .enabled = true,
        .ip_addr = "192.168.1.100",
        .port = 623,              /* IPMI端口 */
        .username = "admin",
        .password = "admin",
        .timeout_ms = 2000
    },

    /* 串口通道配置（PDL层） */
    .serial = {
        .enabled = true,
        .device = "/dev/ttyS2",
        .baudrate = 115200,
        .timeout_ms = 2000
    },

    /* 服务配置（PDL层） */
    .primary_channel = PCL_BMC_CHANNEL_NETWORK,
    .auto_switch = true,
    .retry_count = 3,
    .health_check_interval = 5000,  /* 5秒健康检查 */

    /* GPIO控制 */
    .power_gpio = &gpio_bmc_power,
    .reset_gpio = &gpio_bmc_reset
};

static pcl_bmc_cfg_t *bmc_list[] = {
    &bmc_payload,
    NULL
};

/*===========================================================================
 * 卫星平台接口配置（匹配PDL层satellite_service_config_t）
 *===========================================================================*/

/**
 * 卫星平台接口
 * - 通信接口：CAN0
 * - 功能：接收卫星平台命令，上报载荷状态
 */
static pcl_satellite_cfg_t satellite_platform = {
    /* PCL层扩展字段 */
    .name = "satellite_platform",
    .description = "Satellite platform CAN interface",
    .enabled = true,

    /* PDL层配置字段 */
    .can_device = "can0",
    .can_bitrate = 500000,        /* 500Kbps */
    .heartbeat_interval_ms = 1000,/* 1秒心跳 */
    .cmd_timeout_ms = 1000,

    /* GPIO控制 */
    .power_gpio = NULL,
    .reset_gpio = NULL
};

static pcl_satellite_cfg_t *satellite_list[] = {
    &satellite_platform,
    NULL
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
    .name = "board_temp",
    .description = "Board temperature sensor (TMP75)",
    .enabled = true,

    .interface_type = PCL_HW_INTERFACE_I2C,
    .interface_cfg.i2c = {
        .bus = 1,
        .addr = 0x48,
        .speed = 100000           /* 100kHz */
    },

    .sensor_type = PCL_SENSOR_TYPE_TEMPERATURE,
    .sample_interval_ms = 1000,   /* 1秒采样 */
    .power_gpio = &gpio_sensor_power
};

static pcl_sensor_cfg_t *sensor_list[] = {
    &sensor_board_temp,
    NULL
};

/*===========================================================================
 * 板级配置
 *===========================================================================*/

const pcl_board_config_t pcl_board_ti_am6254_h200_100p_base = {
    .platform = "ti/am6254",
    .product = "H200_100P",
    .version = "base",

    .satellites = (const pcl_satellite_cfg_t **)satellite_list,
    .bmcs = (const pcl_bmc_cfg_t **)bmc_list,
    .mcus = (const pcl_mcu_cfg_t **)mcu_list,
    .sensors = (const pcl_sensor_cfg_t **)sensor_list,
    .storages = NULL,
    .apps = NULL
};
