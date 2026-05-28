# HAL UART 串口示例

演示如何使用 HAL UART 驱动进行串口通信。

---

## 📖 学习目标

- 学习如何打开和配置串口设备
- 学习如何发送数据
- 学习如何接收数据
- 理解串口通信的基本参数

---

## 🚀 快速开始

### 硬件要求

- 串口设备（如 /dev/ttyUSB0 或 /dev/ttyS0）
- USB 转串口适配器（可选）

### 编译运行

```bash
python3 build.py config sample_default
python3 build.py build
./_build/bin/example_hal_uart /dev/ttyUSB0
```

### 预期输出

```
=================================
  HAL UART Example
=================================

Opening UART device: /dev/ttyUSB0
UART device opened successfully

Sending data: Hello, UART!
Data sent: 12 bytes

Waiting for data (5 seconds)...
Received: Hello back!

UART device closed
```

---

## 📝 代码说明

### 打开串口设备

```c
#include "hal_serial.h"

hal_serial_config_t config = {
    .baudrate = 115200,
    .databits = 8,
    .stopbits = 1,
    .parity = 'N',  // No parity
};

void *uart_handle = HAL_Serial_Open("/dev/ttyUSB0", &config);
if (!uart_handle) {
    printf("Failed to open UART device\n");
    return -1;
}
```

### 发送数据

```c
const char *message = "Hello, UART!";
int32_t sent = HAL_Serial_Write(uart_handle, 
                                 (const uint8_t *)message, 
                                 strlen(message));
if (sent < 0) {
    printf("Failed to send data\n");
}
```

### 接收数据

```c
uint8_t buffer[256];
int32_t received = HAL_Serial_Read(uart_handle, buffer, sizeof(buffer) - 1);
if (received > 0) {
    buffer[received] = '\0';  // 添加字符串结束符
    printf("Received: %s\n", buffer);
}
```

---

## 🔧 实验任务

### 任务 1: 回显服务器

实现一个回显服务器，将接收到的数据原样发送回去。

```c
while (1) {
    uint8_t buffer[256];
    int32_t len = HAL_Serial_Read(uart_handle, buffer, sizeof(buffer));
    if (len > 0) {
        HAL_Serial_Write(uart_handle, buffer, len);
    }
}
```

### 任务 2: 修改波特率

尝试不同的波特率：9600, 115200, 921600。

### 任务 3: 二进制数据

发送和接收二进制数据而不是文本。

---

## 🛠️ 测试工具

### 使用 minicom 测试

```bash
# 安装 minicom
sudo apt-get install minicom

# 配置串口
sudo minicom -s
# 选择 Serial port setup
# 设置设备为 /dev/ttyUSB0
# 设置波特率为 115200

# 连接
sudo minicom
```

### 使用 screen 测试

```bash
# 连接串口
screen /dev/ttyUSB0 115200

# 退出: Ctrl+A, 然后按 K
```

### 使用 echo 和 cat 测试

```bash
# 发送数据
echo "Hello" > /dev/ttyUSB0

# 接收数据
cat /dev/ttyUSB0
```

---

## 📚 相关 API

### UART API

- `HAL_Serial_Open()` - 打开串口设备
- `HAL_Serial_Close()` - 关闭串口设备
- `HAL_Serial_Write()` - 发送数据
- `HAL_Serial_Read()` - 接收数据
- `HAL_Serial_Flush()` - 清空缓冲区

### 配置结构

```c
typedef struct {
    uint32_t baudrate;  // 波特率: 9600, 115200, etc.
    uint8_t  databits;  // 数据位: 5, 6, 7, 8
    uint8_t  stopbits;  // 停止位: 1, 2
    char     parity;    // 校验位: 'N', 'E', 'O'
} hal_serial_config_t;
```

---

## 💡 注意事项

1. **权限**: 访问串口设备可能需要权限
   ```bash
   sudo usermod -a -G dialout $USER
   # 注销后重新登录
   ```

2. **设备名**: Linux 上常见的串口设备
   - `/dev/ttyUSB0` - USB 转串口
   - `/dev/ttyS0` - 板载串口
   - `/dev/ttyACM0` - USB CDC 设备

3. **波特率**: 发送和接收端必须使用相同的波特率

4. **缓冲区**: 注意缓冲区大小，避免溢出

---

## 🐛 故障排除

### 找不到串口设备

```bash
# 列出所有串口设备
ls /dev/tty*

# 查看 USB 设备
lsusb

# 查看内核消息
dmesg | grep tty
```

### 权限被拒绝

```bash
# 临时解决（需要 root）
sudo chmod 666 /dev/ttyUSB0

# 永久解决（推荐）
sudo usermod -a -G dialout $USER
# 注销后重新登录
```

### 创建虚拟串口（测试用）

```bash
# 安装 socat
sudo apt-get install socat

# 创建虚拟串口对
socat -d -d pty,raw,echo=0 pty,raw,echo=0
# 输出类似: /dev/pts/2 <-> /dev/pts/3

# 在一个终端运行示例
./_build/bin/example_hal_uart /dev/pts/2

# 在另一个终端测试
cat /dev/pts/3  # 接收
echo "test" > /dev/pts/3  # 发送
```

---

## 📚 下一步

- [05_pdl_satellite](../05_pdl_satellite/) - 学习卫星通信协议
- [06_complete_app](../06_complete_app/) - 完整应用示例

---

**难度**: ⭐⭐ (简单)  
**时间**: 20 分钟  
**硬件**: 需要串口设备
