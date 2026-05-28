# HAL API 参考手册

## CAN驱动接口

### HAL_CAN_Init
初始化CAN驱动。
```c
int32_t HAL_CAN_Init(const hal_can_config_t *config, hal_can_handle_t *handle);
```

### HAL_CAN_Deinit
关闭CAN驱动。
```c
int32_t HAL_CAN_Deinit(hal_can_handle_t *handle);
```

### HAL_CAN_Send
发送CAN帧。
```c
int32_t HAL_CAN_Send(hal_can_handle_t *handle, const hal_can_frame_t *frame, uint32_t timeout_ms);
```

### HAL_CAN_Receive
接收CAN帧。
```c
int32_t HAL_CAN_Receive(hal_can_handle_t *handle, hal_can_frame_t *frame, uint32_t timeout_ms);
```

### HAL_CAN_SetFilter
设置CAN ID过滤器。
```c
int32_t HAL_CAN_SetFilter(hal_can_handle_t *handle, uint32_t filter_id, uint32_t filter_mask);
```

### HAL_CAN_GetStats
获取统计信息。
```c
int32_t HAL_CAN_GetStats(hal_can_handle_t *handle, hal_can_stats_t *stats);
```

## 串口驱动接口

### HAL_Serial_Init
初始化串口驱动。
```c
int32_t HAL_Serial_Init(const hal_serial_config_t *config, hal_serial_handle_t *handle);
```

### HAL_Serial_Deinit
关闭串口驱动。
```c
int32_t HAL_Serial_Deinit(hal_serial_handle_t *handle);
```

### HAL_Serial_Send
发送数据。
```c
int32_t HAL_Serial_Send(hal_serial_handle_t *handle, const uint8_t *data, uint32_t size, uint32_t timeout_ms);
```

### HAL_Serial_Receive
接收数据。
```c
int32_t HAL_Serial_Receive(hal_serial_handle_t *handle, uint8_t *buffer, uint32_t size, uint32_t timeout_ms);
```

### HAL_Serial_Configure
重新配置串口参数。
```c
int32_t HAL_Serial_Configure(hal_serial_handle_t *handle, const hal_serial_config_t *config);
```

## I2C驱动接口

### HAL_I2C_Open
打开I2C设备。
```c
int32_t HAL_I2C_Open(const hal_i2c_config_t *config, hal_i2c_handle_t *handle);
```

### HAL_I2C_Close
关闭I2C设备。
```c
int32_t HAL_I2C_Close(hal_i2c_handle_t handle);
```

### HAL_I2C_Write
写入数据到I2C从设备。
```c
int32_t HAL_I2C_Write(hal_i2c_handle_t handle, uint16_t slave_addr,
                      const uint8_t *buffer, uint32_t size);
```

### HAL_I2C_Read
从I2C从设备读取数据。
```c
int32_t HAL_I2C_Read(hal_i2c_handle_t handle, uint16_t slave_addr,
                     uint8_t *buffer, uint32_t size);
```

### HAL_I2C_WriteReg
写寄存器操作（先写寄存器地址，再写数据）。
```c
int32_t HAL_I2C_WriteReg(hal_i2c_handle_t handle, uint16_t slave_addr,
                         uint8_t reg_addr, const uint8_t *buffer, uint32_t size);
```

### HAL_I2C_ReadReg
读寄存器操作（先写寄存器地址，再读数据）。
```c
int32_t HAL_I2C_ReadReg(hal_i2c_handle_t handle, uint16_t slave_addr,
                        uint8_t reg_addr, uint8_t *buffer, uint32_t size);
```

### HAL_I2C_Transfer
执行I2C传输（支持组合传输）。
```c
int32_t HAL_I2C_Transfer(hal_i2c_handle_t handle, i2c_msg_t *msgs, uint32_t num);
```

## SPI驱动接口

### HAL_SPI_Open
打开SPI设备。
```c
int32_t HAL_SPI_Open(const hal_spi_config_t *config, hal_spi_handle_t *handle);
```

### HAL_SPI_Close
关闭SPI设备。
```c
int32_t HAL_SPI_Close(hal_spi_handle_t handle);
```

### HAL_SPI_Write
SPI写操作。
```c
int32_t HAL_SPI_Write(hal_spi_handle_t handle, const uint8_t *buffer, uint32_t size);
```

### HAL_SPI_Read
SPI读操作。
```c
int32_t HAL_SPI_Read(hal_spi_handle_t handle, uint8_t *buffer, uint32_t size);
```

### HAL_SPI_Transfer
SPI全双工传输。
```c
int32_t HAL_SPI_Transfer(hal_spi_handle_t handle, const uint8_t *tx_buffer,
                         uint8_t *rx_buffer, uint32_t size);
```

### HAL_SPI_TransferMulti
SPI批量传输（支持多段传输）。
```c
int32_t HAL_SPI_TransferMulti(hal_spi_handle_t handle, spi_transfer_t *transfers, uint32_t num);
```

### HAL_SPI_SetConfig
设置SPI配置。
```c
int32_t HAL_SPI_SetConfig(hal_spi_handle_t handle, const hal_spi_config_t *config);
```

详细参数说明请参考头文件 `hal_can.h`、`hal_serial.h`、`hal_i2c.h` 和 `hal_spi.h`。
