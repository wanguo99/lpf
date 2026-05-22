# EMS 平台配置指南

**最后更新**: 2026-05-22  
**维护者**: wanguo

本文档说明如何配置 EMS 的目标平台（架构和操作系统）。

## 配置方式

EMS 支持两种配置方式：

1. **Kconfig 配置**（推荐）：通过 `make menuconfig` 进行交互式配置
2. **环境变量**：通过命令行环境变量临时覆盖配置

### 优先级

当同时使用多种配置方式时，优先级从高到低为：

1. **命令行环境变量**（最高优先级）
   ```bash
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
   ```

2. **Shell 环境变量**
   ```bash
   export ARCH=arm64
   export CROSS_COMPILE=aarch64-linux-gnu-
   make
   ```

3. **Kconfig 配置**（.config 文件）
   ```bash
   make menuconfig
   # 在菜单中选择架构和工具链
   ```

4. **默认值**（最低优先级）
   - 架构：x86_64
   - 操作系统：Linux
   - 工具链：本地编译器

**建议**：
- 开发环境：使用 Kconfig 配置（持久化）
- CI/CD：使用命令行变量（灵活性）
- 临时测试：使用环境变量（方便快捷）

## 架构配置 (ARCH)

### 支持的架构

| 架构 | Kconfig 选项 | ARCH 值 | 说明 |
|------|-------------|---------|------|
| x86_64 | `CONFIG_ARCH_X86_64` | `x86_64` | 64位 Intel/AMD 处理器 |
| ARM32 | `CONFIG_ARCH_ARM32` | `arm` | ARMv7-A 32位处理器 |
| ARM64 | `CONFIG_ARCH_ARM64` | `arm64` | ARMv8-A 64位处理器 (AArch64) |
| RISC-V 64 | `CONFIG_ARCH_RISCV64` | `riscv` | RISC-V 64位处理器 |

### 配置方法

#### 方法 1: 使用 Kconfig（推荐）

```bash
make menuconfig
# 进入 "Target Platform" -> "Target architecture"
# 选择目标架构
```

#### 方法 2: 使用环境变量

```bash
# 临时指定架构
make ARCH=arm64

# 或者导出环境变量
export ARCH=arm64
make
```

## 操作系统配置 (OS)

### 支持的操作系统

| 操作系统 | Kconfig 选项 | OS 值 | 说明 |
|---------|-------------|-------|------|
| Linux | `CONFIG_OS_LINUX` | `linux` | Linux 操作系统，支持 POSIX API |
| Windows | `CONFIG_OS_WINDOWS` | `windows` | Windows 操作系统，使用 Win32 API |
| RTOS | `CONFIG_OS_RTOS` | `rtos` | 实时操作系统 (FreeRTOS, RT-Thread 等) |
| macOS | `CONFIG_OS_MACOS` | `macos` | macOS 操作系统，支持 POSIX + macOS 扩展 |
| Bare Metal | `CONFIG_OS_BARE` | `bare` | 裸机环境，无操作系统 |

### 配置方法

#### 方法 1: 使用 Kconfig（推荐）

```bash
make menuconfig
# 进入 "Target Platform" -> "Target operating system"
# 选择目标操作系统
```

#### 方法 2: 使用环境变量

```bash
# 临时指定操作系统
make OS=rtos

# 或者导出环境变量
export OS=rtos
make
```

### OS 配置的作用

OS 配置主要影响 OSAL (OS Abstraction Layer) 的实现：

- **Linux**: 使用 pthread、socket、epoll 等 POSIX API
- **Windows**: 使用 Win32 线程、Winsock、IOCP 等 API
- **RTOS**: 使用 FreeRTOS/RT-Thread 的任务、信号量、消息队列等 API
- **macOS**: 使用 POSIX API + GCD (Grand Central Dispatch)
- **Bare Metal**: 直接操作硬件，无 OS 抽象层

## 交叉编译配置 (CROSS_COMPILE)

### 配置方法

#### 方法 1: 使用 Kconfig（推荐）

```bash
make menuconfig
# 进入 "Target Platform" -> "Cross-compiler prefix"
# 输入交叉编译工具链前缀
```

Kconfig 会根据选择的架构自动设置默认值：

| 架构 | 默认 CROSS_COMPILE |
|------|-------------------|
| x86_64 | (空，本地编译) |
| ARM32 | `arm-linux-gnueabihf-` |
| ARM64 | `aarch64-linux-gnu-` |
| RISC-V 64 | `riscv64-linux-gnu-` |

#### 方法 2: 使用环境变量

```bash
# 临时指定交叉编译工具链
make CROSS_COMPILE=aarch64-linux-gnu-

# 或者导出环境变量
export CROSS_COMPILE=aarch64-linux-gnu-
make
```

### 工具链命名

设置 `CROSS_COMPILE` 后，构建系统会自动添加后缀：

```makefile
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
...
```

例如，`CROSS_COMPILE=aarch64-linux-gnu-` 会使用：
- `aarch64-linux-gnu-gcc`
- `aarch64-linux-gnu-ar`
- `aarch64-linux-gnu-ld`

## 常见使用场景

### 场景 1: 本地 x86_64 Linux 开发

```bash
make menuconfig
# Target Platform:
#   - Target architecture: x86_64
#   - Target operating system: Linux
#   - Cross-compiler prefix: (留空)

make
```

或者使用环境变量：

```bash
make ARCH=x86_64 OS=linux
```

### 场景 2: ARM64 Linux 交叉编译

```bash
make menuconfig
# Target Platform:
#   - Target architecture: ARM64
#   - Target operating system: Linux
#   - Cross-compiler prefix: aarch64-linux-gnu-

make
```

或者使用环境变量：

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- OS=linux
```

### 场景 3: ARM32 RTOS 开发

```bash
make menuconfig
# Target Platform:
#   - Target architecture: ARM32
#   - Target operating system: RTOS
#   - Cross-compiler prefix: arm-none-eabi-

make
```

或者使用环境变量：

```bash
make ARCH=arm CROSS_COMPILE=arm-none-eabi- OS=rtos
```

### 场景 4: RISC-V 裸机开发

```bash
make menuconfig
# Target Platform:
#   - Target architecture: RISC-V 64-bit
#   - Target operating system: Bare Metal
#   - Cross-compiler prefix: riscv64-unknown-elf-

make
```

或者使用环境变量：

```bash
make ARCH=riscv CROSS_COMPILE=riscv64-unknown-elf- OS=bare
```

## 查看当前配置

### 查看 .config 文件

```bash
grep -E "CONFIG_(ARCH|OS|CROSS_COMPILE)" .config
```

输出示例：

```
CONFIG_ARCH_X86_64=y
CONFIG_ARCH="x86_64"
CONFIG_OS_LINUX=y
CONFIG_OS="linux"
CONFIG_CROSS_COMPILE=""
```

### 查看 Makefile 变量

```bash
make -p 2>/dev/null | grep -E "^(ARCH|OS|CROSS_COMPILE) "
```

输出示例：

```
ARCH = x86_64
OS = linux
CROSS_COMPILE = 
```

## 配置文件管理

### 可用的预定义配置

项目提供以下预定义配置文件（位于 `configs/` 目录）：

| 配置文件 | 架构 | 平台 | 用途 |
|---------|------|------|------|
| `defconfig` | x86_64 | Linux | 默认配置（推荐） |
| `x86_64_full_defconfig` | x86_64 | Linux | 完整功能配置 |
| `x86_64_release_defconfig` | x86_64 | Linux | 生产优化配置 |
| `x86_64_minimal_defconfig` | x86_64 | Linux | 最小化配置 |
| `ccm_h200_am625_debug_defconfig` | arm64 | Linux | CCM 产品调试配置（遗留） |
| `ccm_h200_am625_release_defconfig` | arm64 | Linux | CCM 产品发布配置（遗留） |

使用方法：

```bash
make defconfig                        # 加载默认配置
make x86_64_full_defconfig           # 加载完整功能配置
make ccm_h200_am625_debug_defconfig  # 加载 CCM 调试配置
```

详细说明请参考 [DEFCONFIG_GUIDE.md](DEFCONFIG_GUIDE.md)。

### 保存配置

```bash
# 保存为 defconfig
make savedefconfig

# 保存为特定产品配置
cp .config configs/myproduct_defconfig
```

### 加载配置

```bash
# 加载 defconfig
make defconfig

# 加载特定产品配置
make myproduct_defconfig
# 或者
cp configs/myproduct_defconfig .config
make olddefconfig
```

## 注意事项

1. **工具链兼容性**：确保交叉编译工具链与目标架构匹配
2. **OS 抽象层**：不同 OS 配置需要对应的 OSAL 实现
3. **库依赖**：某些库可能只支持特定的 OS（如 pthread 只支持 POSIX 系统）
4. **测试**：交叉编译后建议在目标平台上进行测试

## 参考

- [BUILD_SYSTEM.md](BUILD_SYSTEM.md) - 构建系统详解
- [BUILD_GUIDE.md](BUILD_GUIDE.md) - 构建指南
- [OSAL_PLATFORM.md](OSAL_PLATFORM.md) - OSAL 平台适配指南
- [DEFCONFIG_GUIDE.md](DEFCONFIG_GUIDE.md) - 配置文件指南
- [INSTALL.md](INSTALL.md) - 安装指南
