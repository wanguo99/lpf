# HAL (硬件抽象层)

## 模块概述

HAL (Hardware Abstraction Layer) 封装硬件驱动，隔离硬件平台差异，为上层提供统一的硬件访问接口。

**设计理念**：
- 唯一允许包含硬件平台相关代码的层
- 必须使用OSAL封装的系统调用
- 支持多平台驱动（Linux/RTOS）
- 硬件抽象层设计，隔离平台差异

**支持的硬件**：
- CAN总线（SocketCAN）
- 串口（UART）
- I2C总线（i2c-dev）
- SPI总线（spidev）
- 网络（Ethernet）

## 主要特性

- **平台隔离**：硬件相关代码隔离在`src/linux/`等平台目录
- **统一接口**：提供跨平台的硬件访问API
- **驱动封装**：CAN、串口等硬件驱动的高层封装
- **配置管理**：硬件配置集中在`include/config/`目录
- **OSAL依赖**：必须使用OSAL封装的系统调用，不直接调用系统API

## 支持的硬件

### CAN总线
- **实现**：基于SocketCAN（Linux）
- **特性**：标准帧/扩展帧、ID过滤、统计信息
- **接口**：`hal_can.h`

### 串口 (UART)
- **实现**：基于termios（Linux）
- **特性**：9600-4000000波特率、原始模式、灵活超时
- **接口**：`hal_serial.h`

### I2C总线
- **实现**：基于i2c-dev（Linux）
- **特性**：7位/10位地址、寄存器读写、组合传输、超时控制
- **接口**：`hal_i2c.h`

### SPI总线
- **实现**：基于spidev（Linux）
- **特性**：MODE 0-3、全双工传输、批量传输、动态配置
- **接口**：`hal_spi.h`

## 编译说明

### 快速开始

```bash
# 在项目根目录使用统一构建脚本
python3 build.py config ccm_h200_100p_am625_debug_defconfig
python3 build.py build
```

HAL 模块会作为核心模块的一部分自动编译。

### 构建配置

HAL 模块通过 Kconfig 配置启用：

```kconfig
CONFIG_HAL=y              # 启用 HAL 层
CONFIG_HAL_CAN=y          # 启用 CAN 驱动
CONFIG_HAL_UART=y         # 启用 UART 驱动
CONFIG_HAL_I2C=y          # 启用 I2C 驱动
CONFIG_HAL_SPI=y          # 启用 SPI 驱动
CONFIG_HAL_GPIO=y         # 启用 GPIO 驱动
CONFIG_HAL_WATCHDOG=y     # 启用看门狗驱动
```

编译输出：
- 库文件：`_build/lib/libhal.a`
- 头文件自动包含在构建路径中

## 模块结构

```
hal/
├── include/                    # 公共接口头文件
│   ├── hal_can.h               # CAN接口
│   ├── hal_serial.h            # 串口接口
│   ├── hal_i2c.h               # I2C接口
│   ├── hal_spi.h               # SPI接口
│   ├── hal_network.h           # 网络接口
│   └── config/                 # HAL配置
│       ├── can_types.h         # CAN类型定义
│       ├── can_config.h        # CAN配置
│       ├── i2c_types.h         # I2C类型定义
│       ├── spi_types.h         # SPI类型定义
│       ├── ethernet_config.h   # 以太网配置
│       └── uart_config.h       # 串口配置
└── src/
    ├── linux/                  # Linux驱动实现
    │   ├── hal_can.c           # SocketCAN驱动
    │   ├── hal_serial.c        # termios串口驱动
    │   ├── hal_i2c.c           # i2c-dev驱动
    │   ├── hal_spi.c           # spidev驱动
    │   └── hal_network.c       # Socket网络驱动
    └── rtems/                  # RTEMS驱动（预留）
```

## 依赖关系

**HAL依赖**：
- OSAL层：操作系统抽象（必须使用OSAL封装的系统调用）
- 系统库：根据平台不同（Linux: pthread, rt）

**被依赖**：
- PDL层：外设驱动层
- Apps层：应用层
- Tests层：测试框架

## 使用示例

### 在其他模块中使用HAL

**CMakeLists.txt配置**：
```cmake
# 链接HAL接口库（获取头文件路径）
target_link_libraries(your_module PUBLIC ems::hal_public_api)

# 链接HAL实现库（运行时链接）
target_link_libraries(your_module PRIVATE ems::hal)
```

**代码中使用**：
```c
#include <hal_can.h>
#include <hal_serial.h>
#include <hal_i2c.h>
#include <hal_spi.h>

int main(void)
{
    /* CAN初始化 */
    hal_can_config_t can_cfg = {
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };
    hal_can_handle_t can_handle;
    HAL_CAN_Init(&can_cfg, &can_handle);
    
    /* 串口初始化 */
    hal_serial_config_t uart_cfg = {
        .baud_rate = 115200,
        .data_bits = 8,
        .parity = HAL_SERIAL_PARITY_NONE,
        .stop_bits = 1
    };
    hal_serial_handle_t uart_handle;
    HAL_Serial_Open("/dev/ttyS0", &uart_cfg, &uart_handle);
    
    /* I2C初始化 */
    hal_i2c_config_t i2c_cfg = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };
    hal_i2c_handle_t i2c_handle;
    HAL_I2C_Open(&i2c_cfg, &i2c_handle);
    
    /* I2C读取传感器 */
    uint8_t sensor_data[2];
    HAL_I2C_ReadReg(i2c_handle, 0x50, 0x00, sensor_data, 2);
    
    /* SPI初始化 */
    hal_spi_config_t spi_cfg = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };
    hal_spi_handle_t spi_handle;
    HAL_SPI_Open(&spi_cfg, &spi_handle);
    
    /* SPI全双工传输 */
    uint8_t tx_buf[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t rx_buf[4];
    HAL_SPI_Transfer(spi_handle, tx_buf, rx_buf, 4);
    
    return 0;
}
```

## 测试

```bash
# 编译测试
./build.sh -d

# 运行HAL测试
./build/bin/unit-test -L HAL              # 运行所有HAL测试
./build/bin/unit-test -m test_hal_can    # 运行CAN测试
./build/bin/unit-test -m test_hal_serial # 运行串口测试
./build/bin/unit-test -m test_hal_i2c    # 运行I2C测试
./build/bin/unit-test -m test_hal_spi    # 运行SPI测试
./build/bin/unit-test -i                 # 交互式菜单
```

**注意**：HAL测试需要实际硬件设备：
- CAN测试：需要 `can0` 或 `vcan0` 设备
- 串口测试：需要 `/dev/ttyS0` 设备
- I2C测试：需要 `/dev/i2c-0` 设备
- SPI测试：需要 `/dev/spidev0.0` 设备

### 设置虚拟CAN设备（用于测试）

```bash
# 加载vcan模块
sudo modprobe vcan

# 创建虚拟CAN设备
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# 验证
ip link show vcan0
```

## 开发指南

### 添加新的硬件驱动

1. 在 `include/` 添加接口头文件（如 `hal_gpio.h`）
2. 在 `include/config/` 添加类型定义（如 `gpio_types.h`）
3. 在 `src/linux/` 添加Linux实现（如 `hal_gpio.c`）
4. 在 `CMakeLists.txt` 的 `HAL_SOURCES` 中添加源文件
5. 在 `tests/hal/` 添加单元测试（如 `test_hal_gpio.c`）
6. 更新 `tests/CMakeLists.txt` 添加测试源文件

### 移植到新平台

1. 创建新的平台目录：`src/<platform>/`（如 `src/rtems/`）
2. 实现HAL接口（参考 `src/linux/` 实现）
3. 修改 `CMakeLists.txt` 添加平台判断：
   ```cmake
   elseif(PLATFORM STREQUAL "rtems")
       set(HAL_SOURCES
           src/rtems/hal_can.c
           src/rtems/hal_serial.c
       )
   ```
4. 运行测试验证移植

### 编码规范（重要）

**必须遵守**：
- ✅ 使用OSAL封装的系统调用：`OSAL_socket()`, `OSAL_open()`, `OSAL_close()`
- ❌ 禁止直接调用：`socket()`, `open()`, `close()`, `memcpy()`, `strlen()`
- ✅ 使用OSAL日志：`LOG_INFO()`, `LOG_ERROR()`
- ❌ 禁止使用：`printf()`, `fprintf()`

**示例**：
```c
/* ✅ 正确 */
int32 sockfd = OSAL_socket(PF_CAN, SOCK_RAW, CAN_RAW);
OSAL_bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
LOG_INFO("HAL_CAN", "CAN initialized");

/* ❌ 错误 */
int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);  // 禁止
bind(sockfd, ...);                                // 禁止
printf("CAN initialized\n");                      // 禁止
```

## 配置说明

### CAN配置

编辑 `include/config/can_config.h`：
```c
#define HAL_CAN_DEFAULT_INTERFACE "can0"
#define HAL_CAN_DEFAULT_BAUDRATE  500000
#define HAL_CAN_RX_BUFFER_SIZE    64
#define HAL_CAN_TX_BUFFER_SIZE    64
```

### 串口配置

编辑 `include/config/uart_config.h`：
```c
#define HAL_UART_DEFAULT_DEVICE   "/dev/ttyS0"
#define HAL_UART_DEFAULT_BAUDRATE 115200
#define HAL_UART_RX_BUFFER_SIZE   256
#define HAL_UART_TX_BUFFER_SIZE   256
```

## 常见问题

**Q: 编译时提示找不到 `<linux/can.h>`？**
```bash
# 安装CAN开发库
sudo apt-get install can-utils libsocketcan-dev
```

**Q: CAN设备无法打开？**
```bash
# 检查CAN驱动
lsmod | grep can
sudo modprobe can
sudo modprobe can_raw

# 检查设备
ip link show can0
```

**Q: 如何添加自定义平台？**
```bash
# 1. 创建平台目录
mkdir -p hal/src/myplatform

# 2. 实现驱动
cp hal/src/linux/hal_can.c hal/src/myplatform/

# 3. 修改CMakeLists.txt
# 添加平台判断和源文件
```

**Q: 如何启用HAL调试日志？**
```c
// 在代码中设置日志级别
OSAL_LogSetLevel(OSAL_LOG_DEBUG);
```

## 设计原则

1. **平台隔离**：HAL是唯一允许包含硬件平台相关代码的层（除OSAL外）
2. **OSAL依赖**：必须使用OSAL封装的系统调用（`OSAL_socket()`, `OSAL_open()`等）
3. **统一接口**：所有平台实现相同的接口
4. **错误处理**：所有函数返回int32状态码
5. **资源管理**：使用句柄管理硬件资源

## 平台支持

当前支持的平台：
- **Linux** (`src/linux/`) - 基于SocketCAN和termios

计划支持的平台：
- **TI AM6254** (`src/ti_am62/`) - 硬件CAN控制器
- **NXP i.MX8** (`src/nxp_imx8/`) - 硬件CAN控制器

## 参考文档

- [HAL详细文档](docs/README.md)
- [架构设计](docs/ARCHITECTURE.md)
- [API参考](docs/API_REFERENCE.md)
- [使用指南](docs/USAGE_GUIDE.md)
- [CAN配置说明](include/config/can_config.h)
- [串口配置说明](include/config/uart_config.h)
- [编码规范](../docs/CODING_STANDARDS.md)
