# HAL CAN 总线示例

演示如何使用 HAL CAN 驱动进行 CAN 总线通信。

---

## 📖 学习目标

- 学习如何打开和配置 CAN 设备
- 学习如何发送 CAN 消息
- 学习如何接收 CAN 消息
- 理解 CAN 总线的基本概念

---

## 🚀 快速开始

### 硬件要求

- CAN 接口（如 can0）
- CAN 收发器硬件

### 配置 CAN 接口

```bash
# 设置 CAN 接口（需要 root 权限）
sudo ip link set can0 type can bitrate 500000
sudo ip link set can0 up

# 查看 CAN 接口状态
ip -details link show can0
```

### 编译运行

```bash
python3 build.py config sample_default
python3 build.py build
./_build/bin/example_hal_can can0
```

### 预期输出

```
=================================
  HAL CAN Example
=================================

Opening CAN device: can0
CAN device opened successfully

Sending CAN message...
  ID: 0x123
  Data: 01 02 03 04 05 06 07 08
Message sent successfully

Waiting for CAN messages (5 seconds)...
Received CAN message:
  ID: 0x456
  DLC: 8
  Data: AA BB CC DD EE FF 00 11

CAN device closed
```

---

## 📝 代码说明

### 打开 CAN 设备

```c
#include "hal_can.h"

hal_can_config_t config = {
    .bitrate = 500000,  // 500 kbps
    .mode = HAL_CAN_MODE_NORMAL,
};

void *can_handle = HAL_CAN_Open("can0", &config);
if (!can_handle) {
    printf("Failed to open CAN device\n");
    return -1;
}
```

### 发送 CAN 消息

```c
hal_can_frame_t frame = {
    .can_id = 0x123,
    .can_dlc = 8,
    .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
};

int32_t ret = HAL_CAN_Write(can_handle, &frame);
if (ret < 0) {
    printf("Failed to send CAN message\n");
}
```

### 接收 CAN 消息

```c
hal_can_frame_t rx_frame;

int32_t ret = HAL_CAN_Read(can_handle, &rx_frame, 5000);  // 超时 5 秒
if (ret > 0) {
    printf("Received CAN message:\n");
    printf("  ID: 0x%03X\n", rx_frame.can_id);
    printf("  DLC: %d\n", rx_frame.can_dlc);
    printf("  Data: ");
    for (int i = 0; i < rx_frame.can_dlc; i++) {
        printf("%02X ", rx_frame.data[i]);
    }
    printf("\n");
}
```

---

## 🔧 实验任务

### 任务 1: 发送自定义消息

修改 CAN ID 和数据，发送自己的消息。

### 任务 2: 循环接收

实现一个循环，持续接收 CAN 消息。

### 任务 3: 过滤器

使用 CAN 过滤器只接收特定 ID 的消息。

```c
hal_can_filter_t filter = {
    .id = 0x123,
    .mask = 0x7FF,  // 标准帧
};
HAL_CAN_SetFilter(can_handle, &filter);
```

---

## 🛠️ 测试工具

### 使用 can-utils 测试

```bash
# 安装 can-utils
sudo apt-get install can-utils

# 发送测试消息
cansend can0 123#0102030405060708

# 接收消息
candump can0

# 生成随机流量
cangen can0 -v
```

---

## 📚 相关 API

### CAN API

- `HAL_CAN_Open()` - 打开 CAN 设备
- `HAL_CAN_Close()` - 关闭 CAN 设备
- `HAL_CAN_Write()` - 发送 CAN 消息
- `HAL_CAN_Read()` - 接收 CAN 消息
- `HAL_CAN_SetFilter()` - 设置过滤器

### CAN 帧结构

```c
typedef struct {
    uint32_t can_id;        // CAN ID
    uint8_t  can_dlc;       // 数据长度 (0-8)
    uint8_t  data[8];       // 数据
} hal_can_frame_t;
```

---

## 💡 注意事项

1. **权限**: 访问 CAN 设备可能需要 root 权限
2. **硬件**: 确保 CAN 接口已正确配置
3. **波特率**: 发送和接收端必须使用相同的波特率
4. **终端电阻**: CAN 总线需要 120Ω 终端电阻

---

## 🐛 故障排除

### 找不到 can0

```bash
# 检查 CAN 接口
ip link show

# 加载 CAN 驱动（如果需要）
sudo modprobe can
sudo modprobe can_raw
sudo modprobe vcan
```

### 创建虚拟 CAN 接口（测试用）

```bash
# 加载虚拟 CAN 驱动
sudo modprobe vcan

# 创建虚拟接口
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# 使用虚拟接口测试
./_build/bin/example_hal_can vcan0
```

---

## 📚 下一步

- [04_hal_uart](../04_hal_uart/) - 学习串口通信
- [05_pdl_satellite](../05_pdl_satellite/) - 学习卫星通信协议

---

**难度**: ⭐⭐⭐ (中等)  
**时间**: 30 分钟  
**硬件**: 需要 CAN 接口
