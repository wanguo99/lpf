# HAL API 参考文档

HAL (Hardware Abstraction Layer) 提供跨平台的硬件访问接口。

---

## 📋 目录

1. [CAN 总线](#can-总线)
2. [串口通信](#串口通信)
3. [I2C 总线](#i2c-总线)
4. [SPI 总线](#spi-总线)
5. [GPIO 控制](#gpio-控制)
6. [看门狗](#看门狗)

---

## CAN 总线

### HAL_CAN_Init

初始化 CAN 总线设备。

```c
int32_t HAL_CAN_Init(
    const hal_can_config_t *config,
    hal_can_handle_t *handle
);
```

**参数**:
- `config`: CAN 配置结构
  - `interface`: CAN 接口名称（如 "can0", "vcan0"）
  - `baudrate`: 波特率（如 500000, 1000000）
  - `rx_timeout`: 接收超时时间（毫秒）
  - `tx_timeout`: 发送超时时间（毫秒）
- `handle`: 返回的 CAN 句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
hal_can_config_t config = {
    .interface = "can0",
    .baudrate = 500000,
    .rx_timeout = 1000,
    .tx_timeout = 1000
};

hal_can_handle_t handle = NULL;
int32_t ret = HAL_CAN_Init(&config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to initialize CAN\n");
    return -1;
}
```

---

### HAL_CAN_Deinit

关闭 CAN 总线设备。

```c
int32_t HAL_CAN_Deinit(hal_can_handle_t handle);
```

**参数**:
- `handle`: CAN 句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

**注意**: 必须调用此函数释放资源。

---

### HAL_CAN_Send

发送 CAN 帧。

```c
int32_t HAL_CAN_Send(
    hal_can_handle_t handle,
    const can_frame_t *frame
);
```

**参数**:
- `handle`: CAN 句柄
- `frame`: CAN 帧结构
  - `can_id`: CAN ID（标准帧或扩展帧）
  - `can_dlc`: 数据长度（0-8）
  - `data[8]`: 数据字节

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_TIMEOUT`: 超时
- `OSAL_ERR_GENERIC`: 其他错误

**示例**:
```c
can_frame_t frame = {
    .can_id = 0x123,
    .can_dlc = 8,
    .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
};

int32_t ret = HAL_CAN_Send(handle, &frame);
if (ret != OSAL_SUCCESS) {
    printf("Failed to send CAN frame\n");
}
```

---

### HAL_CAN_Recv

接收 CAN 帧。

```c
int32_t HAL_CAN_Recv(
    hal_can_handle_t handle,
    can_frame_t *frame,
    int32_t timeout
);
```

**参数**:
- `handle`: CAN 句柄
- `frame`: 接收到的 CAN 帧（输出）
- `timeout`: 超时时间（毫秒）
  - `-1` 或 `OS_PEND`: 永久等待
  - `0`: 非阻塞
  - `> 0`: 超时时间

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_TIMEOUT`: 超时
- `OSAL_ERR_GENERIC`: 其他错误

**示例**:
```c
can_frame_t frame;
int32_t ret = HAL_CAN_Recv(handle, &frame, 1000);
if (ret == OSAL_SUCCESS) {
    printf("Received CAN ID: 0x%X, DLC: %d\n", 
           frame.can_id, frame.can_dlc);
} else if (ret == OSAL_ERR_TIMEOUT) {
    printf("Timeout waiting for CAN frame\n");
}
```

---

### HAL_CAN_SetFilter

设置 CAN 过滤器。

```c
int32_t HAL_CAN_SetFilter(
    hal_can_handle_t handle,
    uint32_t filter_id,
    uint32_t filter_mask
);
```

**参数**:
- `handle`: CAN 句柄
- `filter_id`: 过滤 ID
- `filter_mask`: 过滤掩码

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**说明**:
- 过滤器用于只接收特定 ID 的 CAN 帧
- `filter_mask` 中为 1 的位表示必须匹配
- 例如: `filter_id=0x100`, `filter_mask=0x700` 表示接收 0x100-0x1FF

**示例**:
```c
// 只接收 ID 为 0x100-0x1FF 的帧
HAL_CAN_SetFilter(handle, 0x100, 0x700);
```

---

## 串口通信

### HAL_Serial_Open

打开串口设备。

```c
int32_t HAL_Serial_Open(
    const char *device,
    const hal_serial_config_t *config,
    hal_serial_handle_t *handle
);
```

**参数**:
- `device`: 设备路径（如 "/dev/ttyS0", "/dev/ttyUSB0"）
- `config`: 串口配置
  - `baud_rate`: 波特率（9600, 115200, 921600 等）
  - `data_bits`: 数据位（5, 6, 7, 8）
  - `stop_bits`: 停止位（1, 2）
  - `parity`: 校验位
    - `HAL_SERIAL_PARITY_NONE`: 无校验
    - `HAL_SERIAL_PARITY_ODD`: 奇校验
    - `HAL_SERIAL_PARITY_EVEN`: 偶校验
  - `flow_control`: 流控制
    - `HAL_SERIAL_FLOW_NONE`: 无流控
    - `HAL_SERIAL_FLOW_HW`: 硬件流控
    - `HAL_SERIAL_FLOW_SW`: 软件流控
- `handle`: 返回的串口句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
hal_serial_config_t config = {
    .baud_rate = 115200,
    .data_bits = 8,
    .stop_bits = 1,
    .parity = HAL_SERIAL_PARITY_NONE,
    .flow_control = HAL_SERIAL_FLOW_NONE
};

hal_serial_handle_t handle = NULL;
int32_t ret = HAL_Serial_Open("/dev/ttyUSB0", &config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to open serial port\n");
    return -1;
}
```

---

### HAL_Serial_Close

关闭串口设备。

```c
int32_t HAL_Serial_Close(hal_serial_handle_t handle);
```

**参数**:
- `handle`: 串口句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### HAL_Serial_Write

向串口写入数据。

```c
int32_t HAL_Serial_Write(
    hal_serial_handle_t handle,
    const void *buffer,
    uint32_t size,
    int32_t timeout
);
```

**参数**:
- `handle`: 串口句柄
- `buffer`: 发送缓冲区
- `size`: 数据长度
- `timeout`: 超时时间（毫秒）
  - `-1`: 阻塞模式
  - `0`: 非阻塞模式
  - `> 0`: 超时时间

**返回值**:
- `>= 0`: 实际写入的字节数
- `< 0`: 错误

**示例**:
```c
const char *message = "Hello, UART!\n";
int32_t sent = HAL_Serial_Write(handle, message, strlen(message), -1);
if (sent < 0) {
    printf("Failed to write data\n");
} else {
    printf("Sent %d bytes\n", sent);
}
```

---

### HAL_Serial_Read

从串口读取数据。

```c
int32_t HAL_Serial_Read(
    hal_serial_handle_t handle,
    void *buffer,
    uint32_t size,
    int32_t timeout
);
```

**参数**:
- `handle`: 串口句柄
- `buffer`: 接收缓冲区
- `size`: 缓冲区大小
- `timeout`: 超时时间（毫秒）
  - `-1`: 阻塞模式
  - `0`: 非阻塞模式
  - `> 0`: 超时时间

**返回值**:
- `>= 0`: 实际读取的字节数
- `< 0`: 错误

**示例**:
```c
uint8_t buffer[256];
int32_t received = HAL_Serial_Read(handle, buffer, sizeof(buffer), 1000);
if (received > 0) {
    printf("Received %d bytes\n", received);
} else if (received == 0) {
    printf("Timeout\n");
} else {
    printf("Read error\n");
}
```

---

### HAL_Serial_Flush

刷新串口缓冲区。

```c
int32_t HAL_Serial_Flush(hal_serial_handle_t handle);
```

**参数**:
- `handle`: 串口句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

**说明**: 清空输入和输出缓冲区中的数据。

---

### HAL_Serial_SetConfig

设置串口配置。

```c
int32_t HAL_Serial_SetConfig(
    hal_serial_handle_t handle,
    const hal_serial_config_t *config
);
```

**参数**:
- `handle`: 串口句柄
- `config`: 新的串口配置

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**说明**: 可以在运行时修改波特率等参数。

---

## I2C 总线

### HAL_I2C_Open

打开 I2C 设备。

```c
int32_t HAL_I2C_Open(
    const hal_i2c_config_t *config,
    hal_i2c_handle_t *handle
);
```

**参数**:
- `config`: I2C 配置
  - `device`: 设备路径（如 "/dev/i2c-0"）
  - `timeout`: 传输超时时间（毫秒）
- `handle`: 返回的 I2C 句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
hal_i2c_config_t config = {
    .device = "/dev/i2c-1",
    .timeout = 1000
};

hal_i2c_handle_t handle = NULL;
int32_t ret = HAL_I2C_Open(&config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to open I2C device\n");
    return -1;
}
```

---

### HAL_I2C_Close

关闭 I2C 设备。

```c
int32_t HAL_I2C_Close(hal_i2c_handle_t handle);
```

**参数**:
- `handle`: I2C 句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### HAL_I2C_Write

写入数据到 I2C 从设备。

```c
int32_t HAL_I2C_Write(
    hal_i2c_handle_t handle,
    uint16_t slave_addr,
    const uint8_t *buffer,
    uint32_t size
);
```

**参数**:
- `handle`: I2C 句柄
- `slave_addr`: 从设备地址（7 位地址）
- `buffer`: 数据缓冲区
- `size`: 数据长度

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_TIMEOUT`: 超时
- `OSAL_ERR_GENERIC`: 其他错误

**示例**:
```c
uint8_t data[] = {0x01, 0x02, 0x03};
int32_t ret = HAL_I2C_Write(handle, 0x50, data, sizeof(data));
if (ret != OSAL_SUCCESS) {
    printf("I2C write failed\n");
}
```

---

### HAL_I2C_Read

从 I2C 从设备读取数据。

```c
int32_t HAL_I2C_Read(
    hal_i2c_handle_t handle,
    uint16_t slave_addr,
    uint8_t *buffer,
    uint32_t size
);
```

**参数**:
- `handle`: I2C 句柄
- `slave_addr`: 从设备地址（7 位地址）
- `buffer`: 数据缓冲区（输出）
- `size`: 读取长度

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_TIMEOUT`: 超时
- `OSAL_ERR_GENERIC`: 其他错误

**示例**:
```c
uint8_t buffer[16];
int32_t ret = HAL_I2C_Read(handle, 0x50, buffer, sizeof(buffer));
if (ret == OSAL_SUCCESS) {
    printf("Read %d bytes from I2C device\n", sizeof(buffer));
}
```

---

### HAL_I2C_WriteReg

写入 I2C 寄存器。

```c
int32_t HAL_I2C_WriteReg(
    hal_i2c_handle_t handle,
    uint16_t slave_addr,
    uint8_t reg_addr,
    const uint8_t *buffer,
    uint32_t size
);
```

**参数**:
- `handle`: I2C 句柄
- `slave_addr`: 从设备地址（7 位地址）
- `reg_addr`: 寄存器地址
- `buffer`: 数据缓冲区
- `size`: 数据长度

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**说明**: 先发送寄存器地址，再发送数据。

**示例**:
```c
uint8_t value = 0x42;
// 写入寄存器 0x10
int32_t ret = HAL_I2C_WriteReg(handle, 0x50, 0x10, &value, 1);
```

---

### HAL_I2C_ReadReg

读取 I2C 寄存器。

```c
int32_t HAL_I2C_ReadReg(
    hal_i2c_handle_t handle,
    uint16_t slave_addr,
    uint8_t reg_addr,
    uint8_t *buffer,
    uint32_t size
);
```

**参数**:
- `handle`: I2C 句柄
- `slave_addr`: 从设备地址（7 位地址）
- `reg_addr`: 寄存器地址
- `buffer`: 数据缓冲区（输出）
- `size`: 读取长度

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**说明**: 先发送寄存器地址，再读取数据。

**示例**:
```c
uint8_t value;
// 读取寄存器 0x10
int32_t ret = HAL_I2C_ReadReg(handle, 0x50, 0x10, &value, 1);
if (ret == OSAL_SUCCESS) {
    printf("Register 0x10 = 0x%02X\n", value);
}
```

---

### HAL_I2C_Transfer

执行 I2C 组合传输。

```c
int32_t HAL_I2C_Transfer(
    hal_i2c_handle_t handle,
    i2c_msg_t *msgs,
    uint32_t num
);
```

**参数**:
- `handle`: I2C 句柄
- `msgs`: 传输消息数组
- `num`: 消息数量

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**说明**: 支持复杂的 I2C 传输序列，如重复起始条件。

---

## SPI 总线

### HAL_SPI_Open

打开 SPI 设备。

```c
int32_t HAL_SPI_Open(
    const hal_spi_config_t *config,
    hal_spi_handle_t *handle
);
```

**参数**:
- `config`: SPI 配置
  - `device`: 设备路径（如 "/dev/spidev0.0"）
  - `mode`: SPI 模式（0-3）
    - Mode 0: CPOL=0, CPHA=0
    - Mode 1: CPOL=0, CPHA=1
    - Mode 2: CPOL=1, CPHA=0
    - Mode 3: CPOL=1, CPHA=1
  - `bits_per_word`: 每字位数（通常为 8）
  - `max_speed_hz`: 最大速率（Hz）
  - `timeout`: 传输超时（毫秒）
- `handle`: 返回的 SPI 句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
hal_spi_config_t config = {
    .device = "/dev/spidev0.0",
    .mode = 0,
    .bits_per_word = 8,
    .max_speed_hz = 1000000,  // 1 MHz
    .timeout = 1000
};

hal_spi_handle_t handle = NULL;
int32_t ret = HAL_SPI_Open(&config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to open SPI device\n");
    return -1;
}
```

---

### HAL_SPI_Close

关闭 SPI 设备。

```c
int32_t HAL_SPI_Close(hal_spi_handle_t handle);
```

**参数**:
- `handle`: SPI 句柄

**返回值**:
- `OSAL_SUCCESS`: 成功

---

### HAL_SPI_Write

SPI 写操作。

```c
int32_t HAL_SPI_Write(
    hal_spi_handle_t handle,
    const uint8_t *buffer,
    uint32_t size
);
```

**参数**:
- `handle`: SPI 句柄
- `buffer`: 发送缓冲区
- `size`: 数据长度

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
int32_t ret = HAL_SPI_Write(handle, data, sizeof(data));
```

---

### HAL_SPI_Read

SPI 读操作。

```c
int32_t HAL_SPI_Read(
    hal_spi_handle_t handle,
    uint8_t *buffer,
    uint32_t size
);
```

**参数**:
- `handle`: SPI 句柄
- `buffer`: 接收缓冲区（输出）
- `size`: 数据长度

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
uint8_t buffer[4];
int32_t ret = HAL_SPI_Read(handle, buffer, sizeof(buffer));
```

---

### HAL_SPI_Transfer

SPI 全双工传输。

```c
int32_t HAL_SPI_Transfer(
    hal_spi_handle_t handle,
    const uint8_t *tx_buffer,
    uint8_t *rx_buffer,
    uint32_t size
);
```

**参数**:
- `handle`: SPI 句柄
- `tx_buffer`: 发送缓冲区
- `rx_buffer`: 接收缓冲区（输出）
- `size`: 数据长度

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**说明**: 同时发送和接收数据。

**示例**:
```c
uint8_t tx_data[] = {0x01, 0x02, 0x03, 0x04};
uint8_t rx_data[4];
int32_t ret = HAL_SPI_Transfer(handle, tx_data, rx_data, sizeof(tx_data));
if (ret == OSAL_SUCCESS) {
    printf("Received: %02X %02X %02X %02X\n", 
           rx_data[0], rx_data[1], rx_data[2], rx_data[3]);
}
```

---

### HAL_SPI_TransferMulti

SPI 批量传输。

```c
int32_t HAL_SPI_TransferMulti(
    hal_spi_handle_t handle,
    spi_transfer_t *transfers,
    uint32_t num
);
```

**参数**:
- `handle`: SPI 句柄
- `transfers`: 传输数组
- `num`: 传输数量

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**说明**: 支持多段传输，可以在传输之间保持 CS 信号。

---

### HAL_SPI_SetConfig

设置 SPI 配置。

```c
int32_t HAL_SPI_SetConfig(
    hal_spi_handle_t handle,
    const hal_spi_config_t *config
);
```

**参数**:
- `handle`: SPI 句柄
- `config`: 新的 SPI 配置

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_GENERIC`: 失败

**说明**: 可以在运行时修改 SPI 参数。

---

## GPIO 控制

### HAL_GPIO_Init

初始化 GPIO 引脚。

```c
int32_t HAL_GPIO_Init(
    uint32_t gpio_num,
    const hal_gpio_config_t *config
);
```

**参数**:
- `gpio_num`: GPIO 引脚号
- `config`: GPIO 配置
  - `direction`: GPIO 方向
    - `HAL_GPIO_DIR_INPUT`: 输入模式
    - `HAL_GPIO_DIR_OUTPUT`: 输出模式
  - `initial_level`: 初始电平（输出模式）
    - `HAL_GPIO_LEVEL_LOW`: 低电平
    - `HAL_GPIO_LEVEL_HIGH`: 高电平
  - `edge`: 中断触发模式
    - `HAL_GPIO_EDGE_NONE`: 无中断
    - `HAL_GPIO_EDGE_RISING`: 上升沿触发
    - `HAL_GPIO_EDGE_FALLING`: 下降沿触发
    - `HAL_GPIO_EDGE_BOTH`: 双边沿触发
  - `callback`: 中断回调函数
  - `user_data`: 用户数据

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_PERMISSION`: 权限不足
- `OSAL_EIO`: I/O 错误

**示例**:
```c
// 配置为输出模式
hal_gpio_config_t config = {
    .direction = HAL_GPIO_DIR_OUTPUT,
    .initial_level = HAL_GPIO_LEVEL_LOW,
    .edge = HAL_GPIO_EDGE_NONE,
    .callback = NULL,
    .user_data = NULL
};

int32_t ret = HAL_GPIO_Init(17, &config);
if (ret != OSAL_SUCCESS) {
    printf("Failed to initialize GPIO\n");
    return -1;
}
```

---

### HAL_GPIO_Deinit

释放 GPIO 引脚。

```c
int32_t HAL_GPIO_Deinit(uint32_t gpio_num);
```

**参数**:
- `gpio_num`: GPIO 引脚号

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_EIO`: I/O 错误

---

### HAL_GPIO_SetDirection

设置 GPIO 方向。

```c
int32_t HAL_GPIO_SetDirection(
    uint32_t gpio_num,
    hal_gpio_direction_t direction
);
```

**参数**:
- `gpio_num`: GPIO 引脚号
- `direction`: GPIO 方向

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_EIO`: I/O 错误

---

### HAL_GPIO_GetDirection

获取 GPIO 方向。

```c
int32_t HAL_GPIO_GetDirection(
    uint32_t gpio_num,
    hal_gpio_direction_t *direction
);
```

**参数**:
- `gpio_num`: GPIO 引脚号
- `direction`: GPIO 方向（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_EIO`: I/O 错误

---

### HAL_GPIO_SetLevel

设置 GPIO 输出电平。

```c
int32_t HAL_GPIO_SetLevel(
    uint32_t gpio_num,
    hal_gpio_level_t level
);
```

**参数**:
- `gpio_num`: GPIO 引脚号
- `level`: 输出电平

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_EIO`: I/O 错误

**示例**:
```c
// 设置为高电平
HAL_GPIO_SetLevel(17, HAL_GPIO_LEVEL_HIGH);

// 延时
OSAL_Sleep(1000);

// 设置为低电平
HAL_GPIO_SetLevel(17, HAL_GPIO_LEVEL_LOW);
```

---

### HAL_GPIO_GetLevel

读取 GPIO 输入电平。

```c
int32_t HAL_GPIO_GetLevel(
    uint32_t gpio_num,
    hal_gpio_level_t *level
);
```

**参数**:
- `gpio_num`: GPIO 引脚号
- `level`: 当前电平（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_EIO`: I/O 错误

**示例**:
```c
hal_gpio_level_t level;
int32_t ret = HAL_GPIO_GetLevel(17, &level);
if (ret == OSAL_SUCCESS) {
    printf("GPIO 17 level: %s\n", 
           level == HAL_GPIO_LEVEL_HIGH ? "HIGH" : "LOW");
}
```

---

### HAL_GPIO_SetInterrupt

配置 GPIO 中断。

```c
int32_t HAL_GPIO_SetInterrupt(
    uint32_t gpio_num,
    hal_gpio_edge_t edge,
    hal_gpio_isr_callback_t callback,
    void *user_data
);
```

**参数**:
- `gpio_num`: GPIO 引脚号
- `edge`: 中断触发模式
- `callback`: 中断回调函数
- `user_data`: 用户数据

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_NOT_SUPPORTED`: 不支持中断
- `OSAL_EIO`: I/O 错误

**回调函数原型**:
```c
void gpio_callback(uint32_t gpio_num, hal_gpio_level_t level, void *user_data);
```

**示例**:
```c
void button_callback(uint32_t gpio_num, hal_gpio_level_t level, void *user_data)
{
    printf("Button pressed on GPIO %d, level: %d\n", gpio_num, level);
}

// 配置上升沿中断
HAL_GPIO_SetInterrupt(17, HAL_GPIO_EDGE_RISING, button_callback, NULL);
HAL_GPIO_EnableInterrupt(17);
```

---

### HAL_GPIO_EnableInterrupt

使能 GPIO 中断。

```c
int32_t HAL_GPIO_EnableInterrupt(uint32_t gpio_num);
```

**参数**:
- `gpio_num`: GPIO 引脚号

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_EIO`: I/O 错误

---

### HAL_GPIO_DisableInterrupt

禁用 GPIO 中断。

```c
int32_t HAL_GPIO_DisableInterrupt(uint32_t gpio_num);
```

**参数**:
- `gpio_num`: GPIO 引脚号

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_EIO`: I/O 错误

---

## 看门狗

### HAL_WATCHDOG_Init

初始化看门狗设备。

```c
int32_t HAL_WATCHDOG_Init(
    const hal_watchdog_config_t *config,
    hal_watchdog_handle_t *handle
);
```

**参数**:
- `config`: 看门狗配置
  - `device`: 设备路径（如 "/dev/watchdog"）
  - `timeout_sec`: 超时时间（秒），0 表示使用默认值
  - `enable_on_init`: 初始化时是否立即启用
- `handle`: 返回的看门狗句柄（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
hal_watchdog_config_t config = {
    .device = "/dev/watchdog",
    .timeout_sec = 30,
    .enable_on_init = true
};

hal_watchdog_handle_t handle = NULL;
int32_t ret = HAL_WATCHDOG_Init(&config, &handle);
if (ret != OSAL_SUCCESS) {
    printf("Failed to initialize watchdog\n");
    return -1;
}
```

---

### HAL_WATCHDOG_Deinit

关闭看门狗设备。

```c
int32_t HAL_WATCHDOG_Deinit(hal_watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效

**注意**: 某些硬件看门狗一旦启用就无法关闭。

---

### HAL_WATCHDOG_Kick

喂狗（重置看门狗定时器）。

```c
int32_t HAL_WATCHDOG_Kick(hal_watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
while (1) {
    // 执行任务...
    
    // 定期喂狗
    HAL_WATCHDOG_Kick(handle);
    
    OSAL_Sleep(10000);  // 10 秒
}
```

---

### HAL_WATCHDOG_Enable

启用看门狗。

```c
int32_t HAL_WATCHDOG_Enable(hal_watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_GENERIC`: 失败

---

### HAL_WATCHDOG_Disable

禁用看门狗。

```c
int32_t HAL_WATCHDOG_Disable(hal_watchdog_handle_t handle);
```

**参数**:
- `handle`: 看门狗句柄

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_NOT_SUPPORTED`: 硬件不支持禁用
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_GENERIC`: 失败

**注意**: 某些硬件看门狗一旦启用就无法禁用。

---

### HAL_WATCHDOG_SetTimeout

设置看门狗超时时间。

```c
int32_t HAL_WATCHDOG_SetTimeout(
    hal_watchdog_handle_t handle,
    uint32_t timeout_sec
);
```

**参数**:
- `handle`: 看门狗句柄
- `timeout_sec`: 超时时间（秒）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
// 设置超时时间为 60 秒
HAL_WATCHDOG_SetTimeout(handle, 60);
```

---

### HAL_WATCHDOG_GetTimeout

获取看门狗超时时间。

```c
int32_t HAL_WATCHDOG_GetTimeout(
    hal_watchdog_handle_t handle,
    uint32_t *timeout_sec
);
```

**参数**:
- `handle`: 看门狗句柄
- `timeout_sec`: 返回的超时时间（秒，输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_GENERIC`: 失败

---

### HAL_WATCHDOG_GetTimeleft

获取看门狗剩余时间。

```c
int32_t HAL_WATCHDOG_GetTimeleft(
    hal_watchdog_handle_t handle,
    uint32_t *timeleft_sec
);
```

**参数**:
- `handle`: 看门狗句柄
- `timeleft_sec`: 返回的剩余时间（秒，输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_NOT_SUPPORTED`: 硬件不支持此功能
- `OSAL_ERR_INVALID_PARAM`: 参数无效
- `OSAL_ERR_GENERIC`: 失败

**示例**:
```c
uint32_t timeleft;
int32_t ret = HAL_WATCHDOG_GetTimeleft(handle, &timeleft);
if (ret == OSAL_SUCCESS) {
    printf("Watchdog time left: %u seconds\n", timeleft);
}
```

---

### HAL_WATCHDOG_GetStats

获取看门狗统计信息。

```c
int32_t HAL_WATCHDOG_GetStats(
    hal_watchdog_handle_t handle,
    uint32_t *kick_count
);
```

**参数**:
- `handle`: 看门狗句柄
- `kick_count`: 喂狗次数（输出）

**返回值**:
- `OSAL_SUCCESS`: 成功
- `OSAL_ERR_INVALID_PARAM`: 参数无效

---

## 错误码

| 错误码 | 说明 |
|--------|------|
| `OSAL_SUCCESS` | 成功 |
| `OSAL_ERR_GENERIC` | 一般错误 |
| `OSAL_ERR_INVALID_PARAM` | 参数无效 |
| `OSAL_ERR_TIMEOUT` | 超时 |
| `OSAL_ERR_PERMISSION` | 权限不足 |
| `OSAL_ERR_NOT_SUPPORTED` | 不支持的操作 |
| `OSAL_EIO` | I/O 错误 |

---

## 最佳实践

### 1. 资源管理

始终配对初始化和释放：

```c
// ✅ 正确
hal_can_handle_t handle = NULL;
HAL_CAN_Init(&config, &handle);
// 使用 handle...
HAL_CAN_Deinit(handle);

// ❌ 错误 - 忘记释放
HAL_CAN_Init(&config, &handle);
// 使用 handle...
// 忘记调用 HAL_CAN_Deinit
```

### 2. 错误处理

检查返回值：

```c
// ✅ 正确
int32_t ret = HAL_I2C_Write(handle, addr, data, size);
if (ret != OSAL_SUCCESS) {
    OSAL_LOG_ERROR("I2C write failed: %d", ret);
    return -1;
}

// ❌ 错误 - 不检查返回值
HAL_I2C_Write(handle, addr, data, size);
```

### 3. 超时设置

合理设置超时时间：

```c
// ✅ 正确 - 根据实际需求设置超时
hal_serial_config_t config = {
    .baud_rate = 115200,
    // ...
};
HAL_Serial_Read(handle, buffer, size, 1000);  // 1 秒超时

// ❌ 错误 - 永久阻塞可能导致死锁
HAL_Serial_Read(handle, buffer, size, -1);
```

### 4. 中断处理

中断回调函数应尽快返回：

```c
// ✅ 正确 - 快速处理
void gpio_isr(uint32_t gpio_num, hal_gpio_level_t level, void *user_data)
{
    // 设置标志位
    button_pressed = true;
}

// ❌ 错误 - 在中断中执行耗时操作
void gpio_isr(uint32_t gpio_num, hal_gpio_level_t level, void *user_data)
{
    OSAL_Sleep(1000);  // 不要在中断中休眠
    printf("Button pressed\n");  // 避免在中断中打印
}
```

### 5. 看门狗使用

定期喂狗，避免系统重启：

```c
// ✅ 正确 - 定期喂狗
while (running) {
    process_task();
    HAL_WATCHDOG_Kick(wdt_handle);
    OSAL_Sleep(10000);
}

// ❌ 错误 - 忘记喂狗
while (running) {
    process_task();
    OSAL_Sleep(10000);
    // 忘记喂狗，系统会重启
}
```

---

## 平台支持

| 平台 | CAN | UART | I2C | SPI | GPIO | Watchdog |
|------|-----|------|-----|-----|------|----------|
| Linux | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| macOS | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ |
| Windows | 🚧 | 🚧 | 🚧 | 🚧 | 🚧 | 🚧 |
| RTOS | 🚧 | 🚧 | 🚧 | 🚧 | 🚧 | 🚧 |

**说明**:
- ✅ 完全支持
- 🚧 开发中
- ❌ 不支持

---

## 相关文档

- [OSAL API 参考](../osal/API.md)
- [HAL CAN 示例](../../examples/03_hal_can/)
- [HAL UART 示例](../../examples/04_hal_uart/)
- [开发者指南](../../docs/DEVELOPER_GUIDE.md)
- [架构文档](../../docs/ARCHITECTURE.md)

---

**版本**: 1.0.0  
**最后更新**: 2026-05-29
