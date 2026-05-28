# HAL 使用指南

## CAN驱动使用

### 基本使用
```c
#include "hal_can.h"

hal_can_config_t config = {
    .device = "can0",
    .bitrate = 500000,
    .mode = HAL_CAN_MODE_NORMAL
};

hal_can_handle_t handle;
HAL_CAN_Init(&config, &handle);

/* 发送 */
hal_can_frame_t tx_frame = {
    .can_id = 0x123,
    .can_dlc = 8,
    .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
};
HAL_CAN_Send(&handle, &tx_frame, 1000);

/* 接收 */
hal_can_frame_t rx_frame;
HAL_CAN_Receive(&handle, &rx_frame, 5000);

HAL_CAN_Deinit(&handle);
```

### 设置过滤器
```c
/* 只接收ID为0x100-0x1FF的帧 */
HAL_CAN_SetFilter(&handle, 0x100, 0xFF00);
```

## 串口驱动使用

### 基本使用
```c
#include "hal_serial.h"

hal_serial_config_t config = {
    .device = "/dev/ttyS0",
    .baudrate = 115200,
    .data_bits = 8,
    .stop_bits = 1,
    .parity = 'N'
};

hal_serial_handle_t handle;
HAL_Serial_Init(&config, &handle);

/* 发送 */
const uint8_t data[] = "Hello UART\n";
HAL_Serial_Send(&handle, data, sizeof(data), 1000);

/* 接收 */
uint8_t buffer[128];
int32_t len = HAL_Serial_Receive(&handle, buffer, sizeof(buffer), 5000);

HAL_Serial_Deinit(&handle);
```

## I2C驱动使用

### 基本读写
```c
#include "hal_i2c.h"

hal_i2c_config_t config = {
    .device = "/dev/i2c-0",
    .timeout = 1000
};

hal_i2c_handle_t handle;
HAL_I2C_Open(&config, &handle);

/* 写入数据 */
uint8_t tx_data[] = {0x01, 0x02, 0x03, 0x04};
HAL_I2C_Write(handle, 0x50, tx_data, sizeof(tx_data));

/* 读取数据 */
uint8_t rx_data[4];
HAL_I2C_Read(handle, 0x50, rx_data, sizeof(rx_data));

HAL_I2C_Close(handle);
```

### 寄存器读写
```c
/* 读取传感器寄存器 */
uint8_t sensor_data[2];
HAL_I2C_ReadReg(handle, 0x50, 0x00, sensor_data, 2);

/* 写入配置寄存器 */
uint8_t config_data[] = {0x80, 0x01};
HAL_I2C_WriteReg(handle, 0x50, 0x10, config_data, 2);
```

### 组合传输
```c
/* 先写后读（无STOP） */
i2c_msg_t msgs[2];
uint8_t reg_addr = 0x00;
uint8_t read_buf[4];

msgs[0].addr = 0x50;
msgs[0].flags = 0;              /* 写操作 */
msgs[0].len = 1;
msgs[0].buf = &reg_addr;

msgs[1].addr = 0x50;
msgs[1].flags = I2C_M_RD;       /* 读操作 */
msgs[1].len = 4;
msgs[1].buf = read_buf;

HAL_I2C_Transfer(handle, msgs, 2);
```

## SPI驱动使用

### 基本使用
```c
#include "hal_spi.h"

hal_spi_config_t config = {
    .device = "/dev/spidev0.0",
    .mode = SPI_MODE_0,
    .bits_per_word = 8,
    .max_speed_hz = 1000000,
    .timeout = 1000
};

hal_spi_handle_t handle;
HAL_SPI_Open(&config, &handle);

/* 只写 */
uint8_t tx_data[] = {0x01, 0x02, 0x03, 0x04};
HAL_SPI_Write(handle, tx_data, sizeof(tx_data));

/* 只读 */
uint8_t rx_data[4];
HAL_SPI_Read(handle, rx_data, sizeof(rx_data));

HAL_SPI_Close(handle);
```

### 全双工传输
```c
/* 同时发送和接收 */
uint8_t tx_buf[4] = {0xAA, 0xBB, 0xCC, 0xDD};
uint8_t rx_buf[4];
HAL_SPI_Transfer(handle, tx_buf, rx_buf, 4);
```

### 批量传输
```c
/* 多段传输（CS保持低电平） */
spi_transfer_t xfers[2];

/* 第一段：发送命令 */
uint8_t cmd = 0x03;
xfers[0].tx_buf = &cmd;
xfers[0].rx_buf = NULL;
xfers[0].len = 1;
xfers[0].cs_change = 1;  /* 传输后不释放CS */
xfers[0].delay_usecs = 0;
xfers[0].speed_hz = 0;   /* 使用默认速率 */
xfers[0].bits_per_word = 0;

/* 第二段：读取数据 */
uint8_t data[16];
xfers[1].tx_buf = NULL;
xfers[1].rx_buf = data;
xfers[1].len = 16;
xfers[1].cs_change = 0;  /* 传输后释放CS */
xfers[1].delay_usecs = 0;
xfers[1].speed_hz = 0;
xfers[1].bits_per_word = 0;

HAL_SPI_TransferMulti(handle, xfers, 2);
```

### 动态配置
```c
/* 运行时更改SPI模式和速率 */
hal_spi_config_t new_config = {
    .device = "/dev/spidev0.0",
    .mode = SPI_MODE_3,
    .bits_per_word = 8,
    .max_speed_hz = 2000000,
    .timeout = 1000
};
HAL_SPI_SetConfig(handle, &new_config);
```

更多示例请参考测试代码 `tests/hal/test_hal_can.c`、`tests/hal/test_hal_i2c.c` 和 `tests/hal/test_hal_spi.c`。
