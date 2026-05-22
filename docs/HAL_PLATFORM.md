# HAL 平台适配指南

**最后更新**: 2026-05-22  
**维护者**: wanguo

## 概述

HAL (Hardware Abstraction Layer) 支持多硬件平台适配，通过 Kconfig 配置系统自动选择对应的驱动实现。

## 支持的硬件平台

HAL 支持以下硬件平台：

| 平台类型 | Kconfig 选项 | 源码目录 | 说明 |
|---------|-------------|----------|------|
| Generic Linux | `CONFIG_HAL_PLATFORM_GENERIC_LINUX` | `src/generic-linux/` | 通用 Linux 平台（默认） |
| Raspberry Pi | `CONFIG_HAL_PLATFORM_RASPBERRY_PI` | `src/raspberry-pi/` | 树莓派平台 |
| BeagleBone | `CONFIG_HAL_PLATFORM_BEAGLEBONE` | `src/beaglebone/` | BeagleBone 平台 |
| Custom | `CONFIG_HAL_PLATFORM_CUSTOM` | `src/custom/` | 自定义平台 |

## 支持的驱动模块

HAL 提供以下硬件驱动模块：

| 驱动 | 默认 | Kconfig 选项 | 功能 | 子选项 |
|------|------|-------------|------|--------|
| **CAN** | Y | `CONFIG_HAL_CAN` | CAN 总线驱动 | `HAL_CAN_FILTER`, `HAL_CAN_ERROR_HANDLING`, `HAL_CAN_LOOPBACK` |
| **UART** | Y | `CONFIG_HAL_UART` | 串口驱动 | `HAL_UART_FLOW_CONTROL`, `HAL_UART_RS485` |
| **I2C** | N | `CONFIG_HAL_I2C` | I2C 总线驱动 | `HAL_I2C_SLAVE`, `HAL_I2C_10BIT_ADDR` |
| **SPI** | N | `CONFIG_HAL_SPI` | SPI 总线驱动 | `HAL_SPI_DMA`, `HAL_SPI_SLAVE` |
| **GPIO** | Y | `CONFIG_HAL_GPIO` | GPIO 驱动 | `HAL_GPIO_INTERRUPT`, `HAL_GPIO_DEBOUNCE` |
| **Watchdog** | Y | `CONFIG_HAL_WATCHDOG` | 看门狗驱动 | `HAL_WATCHDOG_PRETIMEOUT` |

## 配置方法

### 方法 1: 使用 menuconfig

```bash
make menuconfig
```

在菜单中选择：
1. `HAL (Hardware Abstraction Layer)` -> `Hardware platform` -> 选择目标平台
2. 启用/禁用所需的驱动模块

### 方法 2: 使用 defconfig

创建自定义的 defconfig 文件，例如 `configs/rpi_defconfig`：

```kconfig
CONFIG_HAL=y
CONFIG_HAL_PLATFORM_RASPBERRY_PI=y
CONFIG_HAL_CAN=y
CONFIG_HAL_UART=y
CONFIG_HAL_GPIO=y
CONFIG_HAL_WATCHDOG=y
```

然后使用：

```bash
make rpi_defconfig
```

## 平台目录选择机制

HAL 根据 Kconfig 配置自动选择硬件平台实现目录。

### Makefile 实现

在 `core/hal/Makefile` 中：

```makefile
ifeq ($(CONFIG_HAL_PLATFORM_GENERIC_LINUX),y)
  HAL_PLATFORM_DIR := generic-linux
else ifeq ($(CONFIG_HAL_PLATFORM_RASPBERRY_PI),y)
  HAL_PLATFORM_DIR := raspberry-pi
else ifeq ($(CONFIG_HAL_PLATFORM_BEAGLEBONE),y)
  HAL_PLATFORM_DIR := beaglebone
else ifeq ($(CONFIG_HAL_PLATFORM_CUSTOM),y)
  HAL_PLATFORM_DIR := custom
else
  HAL_PLATFORM_DIR := generic-linux  # 默认
endif
```

### 条件编译

驱动源文件根据配置选择性编译：

```makefile
hal-objs += $(if $(CONFIG_HAL_CAN),src/$(HAL_PLATFORM_DIR)/hal_can_linux.o)
hal-objs += $(if $(CONFIG_HAL_UART),src/$(HAL_PLATFORM_DIR)/hal_serial_linux.o)
hal-objs += $(if $(CONFIG_HAL_I2C),src/$(HAL_PLATFORM_DIR)/hal_i2c_linux.o)
hal-objs += $(if $(CONFIG_HAL_SPI),src/$(HAL_PLATFORM_DIR)/hal_spi_linux.o)
hal-objs += $(if $(CONFIG_HAL_GPIO),src/$(HAL_PLATFORM_DIR)/hal_gpio_linux.o)
hal-objs += $(if $(CONFIG_HAL_WATCHDOG),src/$(HAL_PLATFORM_DIR)/hal_watchdog_linux.o)
```

## 源码目录结构

### 当前实现

```
core/hal/src/
├── generic-linux/      # 通用 Linux 实现 - 已实现
│   ├── hal_can_linux.c       # SocketCAN 实现
│   ├── hal_serial_linux.c    # termios 串口实现
│   ├── hal_i2c_linux.c       # i2c-dev 实现
│   ├── hal_spi_linux.c       # spidev 实现
│   ├── hal_gpio_linux.c      # sysfs GPIO 实现
│   └── hal_watchdog_linux.c  # /dev/watchdog 实现
└── macos/              # macOS 实现 - 部分实现
    ├── hal_can_macos.c
    ├── hal_serial_macos.c
    ├── hal_i2c_macos.c
    ├── hal_spi_macos.c
    ├── hal_gpio_macos.c
    └── hal_watchdog_macos.c
```

**注意**：当前主要实现了 generic-linux 平台（6个驱动），其他平台目录为预留。

### 预留目录（未实现）

```
core/hal/src/
├── raspberry-pi/       # 树莓派实现 - 预留
├── beaglebone/         # BeagleBone 实现 - 预留
└── custom/             # 自定义平台 - 预留
```

## 导出的头文件列表

HAL 通过 `header-y` 机制导出以下头文件到 staging 目录：

### 驱动接口头文件（6个）
- `hal_can.h` - CAN 总线驱动接口
- `hal_serial.h` - 串口驱动接口
- `hal_i2c.h` - I2C 总线驱动接口
- `hal_spi.h` - SPI 总线驱动接口
- `hal_gpio.h` - GPIO 驱动接口
- `hal_watchdog.h` - 看门狗驱动接口

### 配置头文件（6个）
- `config/hal_can_config.h` - CAN 配置参数
- `config/hal_serial_config.h` - 串口配置参数
- `config/hal_i2c_config.h` - I2C 配置参数
- `config/hal_spi_config.h` - SPI 配置参数
- `config/hal_gpio_config.h` - GPIO 配置参数
- `config/hal_watchdog_config.h` - 看门狗配置参数

## 驱动实现说明

### Generic Linux 平台

| 驱动 | 实现方式 | 内核接口 |
|------|---------|---------|
| **CAN** | SocketCAN | `/sys/class/net/can*` |
| **UART** | termios | `/dev/ttyS*`, `/dev/ttyUSB*` |
| **I2C** | i2c-dev | `/dev/i2c-*` |
| **SPI** | spidev | `/dev/spidev*` |
| **GPIO** | sysfs | `/sys/class/gpio/` |
| **Watchdog** | watchdog | `/dev/watchdog` |

### Raspberry Pi 平台（预留）

针对树莓派硬件优化：
- 使用 BCM2835/BCM2711 硬件接口
- 优化的 GPIO 驱动（直接寄存器访问）
- 硬件 PWM 支持
- 硬件 SPI/I2C 支持

### BeagleBone 平台（预留）

针对 TI AM335x 处理器优化：
- PRU (Programmable Real-time Unit) 支持
- 优化的工业接口
- 实时性能优化

## 移植新平台

### 步骤 1: 创建源码目录

```bash
mkdir -p core/hal/src/newplatform
```

### 步骤 2: 实现驱动接口

至少需要实现以下驱动（根据需求）：

- `hal_can_*.c` - CAN 驱动
- `hal_serial_*.c` - 串口驱动
- `hal_gpio_*.c` - GPIO 驱动
- `hal_watchdog_*.c` - 看门狗驱动

每个驱动必须实现对应头文件中定义的接口。

### 步骤 3: 更新 Kconfig

在 `core/hal/Kconfig` 中添加新平台选项：

```kconfig
config HAL_PLATFORM_NEWPLATFORM
    bool "New Platform"
    help
      Support for New Platform hardware.
      Optimized drivers for specific hardware features.
```

### 步骤 4: 更新 Makefile

在 `core/hal/Makefile` 中添加：

```makefile
ifeq ($(CONFIG_HAL_PLATFORM_NEWPLATFORM),y)
  HAL_PLATFORM_DIR := newplatform
endif
```

### 步骤 5: 实现驱动

参考 `generic-linux` 平台的实现，确保：
- 使用 OSAL 接口（不直接调用系统调用）
- 实现统一的错误处理
- 提供完整的功能支持

## 编译标志

HAL Makefile 会根据配置自动添加以下编译标志：

```makefile
# 包含路径
ccflags-y += -I$(src)/include
ccflags-y += -I$(src)/include/config
ccflags-y += -I$(srctree)/core/osal/include
ccflags-y += -I$(srctree)/include

# 调试标志
ccflags-$(CONFIG_DEBUG_HAL) += -DDEBUG_HAL

# 驱动特定标志
CFLAGS_hal_can_linux.o := -DHAL_CAN_VERSION=\"1.0\"
```

## 链接依赖

HAL 依赖 OSAL 库：

```makefile
hal-ldflags += -L$(LIB_DIR) -losal
```

确保在链接 HAL 库时，OSAL 库已经构建完成。

## 测试

### 测试不同平台配置

```bash
# 测试 Generic Linux 平台
make menuconfig
# 选择: HAL -> Hardware platform -> Generic Linux
make core/hal/

# 测试 Raspberry Pi 平台
make menuconfig
# 选择: HAL -> Hardware platform -> Raspberry Pi
make core/hal/
```

### 验证平台目录选择

```bash
make core/hal/ V=1 | grep "src/.*/hal_can"
```

应该看到正确的平台目录（generic-linux/raspberry-pi/beaglebone/custom）。

### 测试驱动功能

```bash
# 编译测试程序
make products/ccm/apps/

# 在目标板上运行
./bin/ccm_collector
```

## 常见问题

### Q: 如何查看当前的平台配置？

```bash
grep -E "CONFIG_HAL_PLATFORM_" .config
```

### Q: 为什么编译时找不到驱动源文件？

检查对应的平台目录是否存在：

```bash
ls -la core/hal/src/generic-linux/  # 或其他平台
```

如果目录不存在，说明该平台尚未实现，需要先移植。

### Q: 如何添加平台特定的编译标志？

在 `core/hal/Makefile` 中添加：

```makefile
ifeq ($(CONFIG_HAL_PLATFORM_RASPBERRY_PI),y)
  ccflags-y += -DRPI_SPECIFIC_FLAG
  hal-ldflags += -lbcm2835
endif
```

### Q: 如何在代码中检测平台？

使用 Kconfig 生成的宏：

```c
#if defined(CONFIG_HAL_PLATFORM_GENERIC_LINUX)
    // Generic Linux 特定代码
#elif defined(CONFIG_HAL_PLATFORM_RASPBERRY_PI)
    // Raspberry Pi 特定代码
#elif defined(CONFIG_HAL_PLATFORM_BEAGLEBONE)
    // BeagleBone 特定代码
#endif
```

### Q: HAL 和 OSAL 的区别是什么？

- **OSAL** (Operating System Abstraction Layer)：操作系统抽象层
  - 抽象不同操作系统的差异（Linux/Windows/RTOS/Bare Metal）
  - 提供线程、互斥锁、信号量、网络等系统级接口
  - 源码目录按 OS 类型组织（posix/win32/rtos/bare）

- **HAL** (Hardware Abstraction Layer)：硬件抽象层
  - 抽象不同硬件平台的差异（Generic Linux/Raspberry Pi/BeagleBone）
  - 提供 CAN、UART、I2C、SPI、GPIO 等硬件驱动接口
  - 源码目录按硬件平台组织（generic-linux/raspberry-pi/beaglebone）
  - 依赖 OSAL 提供的系统接口

## 参考

- [OSAL_PLATFORM.md](OSAL_PLATFORM.md) - OSAL 平台适配指南
- [PLATFORM.md](PLATFORM.md) - 平台配置总体说明
- [BUILD_SYSTEM.md](BUILD_SYSTEM.md) - 构建系统详解
- [BUILD_GUIDE.md](BUILD_GUIDE.md) - 构建指南
