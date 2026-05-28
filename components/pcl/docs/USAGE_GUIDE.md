# PCL 使用指南

## 基本使用

```c
#include "pcl_api.h"

int main(void)
{
    /* 初始化 */
    HW_Config_Init();
    HW_Config_RegisterAll();
    
    /* 选择配置 */
    const pcl_board_config_t *board = HW_Config_SelectDefault();
    
    /* 打印配置 */
    HW_Config_Print(board);
    
    return 0;
}
```

## 查找MCU外设并初始化

```c
/* 获取板级配置 */
const pcl_board_config_t *board = HW_Config_GetBoard();

/* 查找MCU外设配置 */
const pcl_mcu_cfg_t *mcu_cfg = XCONFIG_HW_FindMCU(board, "stm32_mcu");
if (mcu_cfg == NULL) {
    LOG_ERROR("Main", "MCU配置未找到");
    return -1;
}

/* 根据接口类型初始化 */
if (mcu_cfg->interface_type == HW_INTERFACE_UART) {
    /* 使用UART配置初始化MCU */
    hal_serial_config_t serial_cfg = {
        .device = mcu_cfg->interface_cfg.uart.device,
        .baudrate = mcu_cfg->interface_cfg.uart.baudrate,
        .data_bits = mcu_cfg->interface_cfg.uart.data_bits,
        .stop_bits = mcu_cfg->interface_cfg.uart.stop_bits,
        .parity = mcu_cfg->interface_cfg.uart.parity
    };
    HAL_Serial_Init(&serial_cfg, &serial_handle);
} else if (mcu_cfg->interface_type == HW_INTERFACE_CAN) {
    /* 使用CAN配置初始化MCU */
    hal_can_config_t can_cfg = {
        .device = mcu_cfg->interface_cfg.can.device,
        .bitrate = mcu_cfg->interface_cfg.can.bitrate
    };
    HAL_CAN_Init(&can_cfg, &can_handle);
}
```

## 查找BMC外设并初始化双通道

```c
/* 查找BMC外设配置 */
const pcl_bmc_cfg_t *bmc_cfg = XCONFIG_HW_FindBMC(board, "payload_bmc");

/* 初始化主通道（以太网） */
if (bmc_cfg->primary_channel.type == HW_INTERFACE_ETHERNET) {
    /* 初始化以太网连接 */
    connect_ethernet(bmc_cfg->primary_channel.cfg.interface,
                     bmc_cfg->primary_channel.cfg.ip_addr,
                     bmc_cfg->primary_channel.cfg.port);
}

/* 初始化备份通道（串口） */
if (bmc_cfg->backup_channel.type == HW_INTERFACE_UART) {
    hal_serial_config_t serial_cfg = {
        .device = bmc_cfg->backup_channel.cfg.device,
        .baudrate = bmc_cfg->backup_channel.cfg.baudrate
    };
    HAL_Serial_Init(&serial_cfg, &backup_handle);
}
```

## 遍历所有传感器外设

```c
const pcl_board_config_t *board = HW_Config_GetBoard();

for (uint32_t i = 0; i < board->sensor_count; i++) {
    const pcl_sensor_cfg_t *sensor = board->sensors[i];
    
    if (!sensor->enabled) {
        continue;
    }
    
    LOG_INFO("Main", "传感器: %s", sensor->name);
    
    /* 根据接口类型初始化传感器 */
    if (sensor->interface_type == HW_INTERFACE_I2C) {
        /* 初始化I2C传感器 */
        init_i2c_sensor(sensor->interface_cfg.i2c.device,
                        sensor->interface_cfg.i2c.slave_addr);
    } else if (sensor->interface_type == HW_INTERFACE_SPI) {
        /* 初始化SPI传感器 */
        init_spi_sensor(sensor->interface_cfg.spi.device,
                        sensor->interface_cfg.spi.cs_pin);
    }
}
```

## 添加新外设配置

### 步骤1：定义外设配置

```c
/* 新增MCU外设 */
static pcl_mcu_cfg_t mcu_new = {
    .name = "new_mcu",
    .enabled = true,
    .interface_type = HW_INTERFACE_CAN,
    .interface_cfg.can = {
        .device = "can1",
        .bitrate = 500000,
        .tx_id = 0x400,
        .rx_id = 0x500
    },
    .cmd_timeout_ms = 500,
    .retry_count = 3,
    .enable_crc = true
};
```

### 步骤2：添加到外设列表

```c
static pcl_mcu_cfg_t *mcu_list[] = {
    &mcu_stm32,
    &mcu_new  /* 添加新MCU */
};
```

### 步骤3：更新板级配置

```c
const pcl_board_config_t pcl_xxx = {
    /* ... */
    .mcus = mcu_list,
    .mcu_count = sizeof(mcu_list) / sizeof(mcu_list[0]),
    /* ... */
};
```

完整的使用示例请参考 `pcl/examples/pcl_example.c`。
