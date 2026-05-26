# EMS 配置指南

## 配置系统概述

EMS 使用 Kconfig 配置系统，提供灵活的模块裁剪和平台适配能力。

## 配置方式

### 1. 图形化配置

```bash
make menuconfig
```

主要配置项：
- **Build Options** - 构建选项（优化级别、库类型）
- **Target Platform** - 目标平台（架构、操作系统、工具链）
- **OSAL** - 操作系统抽象层
- **HAL** - 硬件抽象层
- **PCL** - 协议控制层
- **PDL** - 协议数据层
- **ACL** - 应用控制层
- **Products** - 产品应用
- **Test Configuration** - 测试配置

### 2. 使用预定义配置

```bash
# 查看可用配置
ls configs/*_defconfig

# 加载配置
make <config>_defconfig
```

### 3. 命令行配置

```bash
# 修改配置项
scripts/kconfig/conf --set-val CONFIG_OSAL y Kconfig
scripts/kconfig/conf --set-val CONFIG_DEBUG y Kconfig
```

## 平台配置

### 架构选择

```
Target Platform
  -> Target architecture
     ( ) x86_64
     ( ) ARM32 (ARMv7-A)
     (X) ARM64 (ARMv8-A / AArch64)
     ( ) RISC-V 64-bit
```

### 操作系统

```
Target Platform
  -> Target operating system
     (X) Linux
     ( ) Windows
     ( ) RTOS
     ( ) macOS
     ( ) Bare Metal
```

### 交叉编译

```
Target Platform
  -> Cross-compiler prefix
     (aarch64-linux-gnu-) Cross-compiler prefix
```

常用工具链前缀：
- ARM64: `aarch64-linux-gnu-`
- ARM32: `arm-linux-gnueabihf-`
- RISC-V: `riscv64-linux-gnu-`

## 模块配置

### OSAL（操作系统抽象层）

```
OSAL (Operating System Abstraction Layer)
  [*] Enable OSAL
  [*]   Build static library
  [*]   Build shared library
  
  OSAL Features
    [*] File operations
    [*] Network support
    [*] IPC support
    [*] Thread support
    [*] Time operations
    [*] Signal handling
```

### HAL（硬件抽象层）

```
HAL (Hardware Abstraction Layer)
  [*] Enable HAL
  [*]   Build static library
  [*]   Build shared library
  
  HAL Drivers
    [*] CAN bus support
    [*] UART support
    [*] I2C support
    [*] SPI support
    [*] GPIO support
    [*] Watchdog support
```

### PDL（协议数据层）

```
PDL (Protocol Data Layer)
  [*] Enable PDL
  [*]   Build static library
  [*]   Build shared library
  
  PDL Features
    [*] Watchdog support
    [*] Satellite support
    [*] BMC support
    [*] MCU support
```

## 构建选项

### 优化级别

```
Build Options
  -> Optimization level
     ( ) No optimization (-O0)
     ( ) Optimize for size (-Os)
     (X) Optimize (-O2)
     ( ) Optimize more (-O3)
```

### 库类型

```
Build Options
  -> Library build type
     [*] Build static libraries
     [*] Build dynamic libraries
```

### 调试选项

```
Build Options
  [*] Debug build
  [ ] Enable profiling
  [ ] Enable coverage
```

## 测试配置

```
Test Configuration
  [*] Build testing framework
  
  Test Types
    [*] Unit tests
    [*] Performance tests
    [*] Stress tests
    [*] System tests
```

## 配置文件管理

### 保存配置

```bash
# 保存最小化配置
make savedefconfig

# 配置保存到 defconfig 文件
cp defconfig configs/my_custom_defconfig
```

### 对比配置

```bash
# 查看与默认配置的差异
scripts/kconfig/conf --savedefconfig=defconfig.new Kconfig
diff configs/defconfig defconfig.new
```

### 配置文件格式

```kconfig
# 注释
CONFIG_OSAL=y
CONFIG_OSAL_BUILD_STATIC=y
CONFIG_OSAL_BUILD_SHARED=y
CONFIG_OSAL_FILE=y
CONFIG_OSAL_NETWORK=y
# CONFIG_HAL is not set
```

## 常见配置场景

### 开发环境（x86_64）

```bash
make x86_64_full_defconfig
# 启用所有功能和调试选项
```

### 生产环境（ARM64）

```bash
make arm64_minimal_defconfig
# 最小功能集，优化体积和性能
```

### 测试环境

```bash
make x86_64_test_defconfig
# 启用所有测试套件
```

## 配置验证

```bash
# 查看当前配置
cat .config | grep "^CONFIG"

# 验证配置完整性
make olddefconfig

# 检查配置冲突
make menuconfig
# 查看是否有警告或错误
```
