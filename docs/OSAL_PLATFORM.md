# OSAL 平台适配指南

## 概述

OSAL (Operating System Abstraction Layer) 支持多平台、多架构的适配，通过 Kconfig 配置系统自动选择对应的源码实现。

## 平台配置

### 支持的操作系统

OSAL 支持以下操作系统：

| OS 类型 | Kconfig 选项 | 源码目录 | 说明 |
|---------|-------------|----------|------|
| Linux | `CONFIG_OS_LINUX` | `src/posix/` | POSIX 兼容系统 |
| macOS | `CONFIG_OS_MACOS` | `src/posix/` | POSIX 兼容系统 |
| Windows | `CONFIG_OS_WINDOWS` | `src/win32/` | Win32 API |
| RTOS | `CONFIG_OS_RTOS` | `src/rtos/` | 实时操作系统 |
| Bare Metal | `CONFIG_OS_BARE` | `src/bare/` | 裸机环境 |

### 支持的架构

OSAL 支持以下处理器架构：

| 架构 | Kconfig 选项 | 位宽 | 说明 |
|------|-------------|------|------|
| x86_64 | `CONFIG_ARCH_X86_64` | 64-bit | Intel/AMD 64位 |
| ARM32 | `CONFIG_ARCH_ARM32` | 32-bit | ARM 32位 |
| ARM64 | `CONFIG_ARCH_ARM64` | 64-bit | ARM 64位 (AArch64) |
| RISC-V 64 | `CONFIG_ARCH_RISCV64` | 64-bit | RISC-V 64位 |

## 配置方法

### 方法 1: 使用 menuconfig

```bash
make menuconfig
```

在菜单中选择：
1. `Target Platform` -> `Target operating system` -> 选择目标 OS
2. `Target Platform` -> `Target architecture` -> 选择目标架构

### 方法 2: 使用 defconfig

创建自定义的 defconfig 文件，例如 `configs/arm64_rtos_defconfig`：

```kconfig
CONFIG_OS_RTOS=y
CONFIG_ARCH_ARM64=y
CONFIG_CROSS_COMPILE="aarch64-none-elf-"
```

然后使用：

```bash
make arm64_rtos_defconfig
```

### 方法 3: 环境变量覆盖

环境变量优先级高于 Kconfig 配置：

```bash
# 交叉编译 ARM64 + RTOS
ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- make
```

## OSAL 平台相关配置

### 自动配置

根据 OS 和 ARCH 配置，OSAL 会自动设置以下配置：

```kconfig
# OS 相关
CONFIG_OSAL_OS_POSIX=y      # Linux/macOS 自动启用
CONFIG_OSAL_OS_WIN32=y      # Windows 自动启用
CONFIG_OSAL_OS_RTOS=y       # RTOS 自动启用
CONFIG_OSAL_OS_BARE=y       # Bare Metal 自动启用

# 架构相关
CONFIG_OSAL_ARCH_32BIT=y    # ARM32 自动启用
CONFIG_OSAL_ARCH_64BIT=y    # x86_64/ARM64/RISC-V64 自动启用
```

### 功能限制

某些功能在特定平台上不可用：

| 功能 | Linux/macOS | Windows | RTOS | Bare Metal |
|------|-------------|---------|------|------------|
| 文件 I/O | ✓ | ✓ | ✓ | ✗ |
| 网络 | ✓ | ✓ | 部分 | ✗ |
| 信号处理 | ✓ | ✗ | ✗ | ✗ |
| 共享库 | ✓ | ✓ | ✗ | ✗ |

## 编译标志

OSAL Makefile 会根据配置自动添加以下编译标志：

```makefile
# OS 相关宏
-DOSAL_OS_POSIX      # POSIX 平台
-DOSAL_OS_WIN32      # Windows 平台
-DOSAL_OS_RTOS       # RTOS 平台
-DOSAL_OS_BARE       # 裸机平台

# 架构相关宏
-DOSAL_ARCH_32BIT    # 32 位架构
-DOSAL_ARCH_64BIT    # 64 位架构

# 配置宏
-DOSAL_MAX_TASKS=128
-DOSAL_MAX_QUEUES=64
-DOSAL_MAX_MUTEXES=64
```

## 平台检测头文件

`osal_platform.h` 提供了统一的平台检测宏：

```c
/* OS 检测 */
#if defined(OSAL_OS_POSIX)
    #define OSAL_PLATFORM_POSIX
#elif defined(OSAL_OS_WIN32)
    #define OSAL_PLATFORM_WINDOWS
#elif defined(OSAL_OS_RTOS)
    #define OSAL_PLATFORM_RTOS
#elif defined(OSAL_OS_BARE)
    #define OSAL_PLATFORM_BARE
#endif

/* 架构检测 */
#if defined(OSAL_ARCH_64BIT)
    #define OSAL_ARCH_BITS 64
#elif defined(OSAL_ARCH_32BIT)
    #define OSAL_ARCH_BITS 32
#endif

/* 字节序检测 */
#define OSAL_LITTLE_ENDIAN 1  // 或 0
#define OSAL_BIG_ENDIAN 0     // 或 1
```

## 源码目录结构

```
core/osal/src/
├── posix/          # POSIX 实现 (Linux/macOS)
│   ├── ipc/        # 进程间通信
│   ├── lib/        # 基础库
│   ├── net/        # 网络
│   ├── sys/        # 系统调用
│   └── util/       # 工具函数
├── win32/          # Windows 实现
├── rtos/           # RTOS 实现
├── bare/           # 裸机实现
└── common/         # 通用代码（所有平台共享）
```

## 移植新平台

### 步骤 1: 创建源码目录

```bash
mkdir -p core/osal/src/newos/{ipc,lib,net,sys,util}
```

### 步骤 2: 实现必需的接口

至少需要实现以下模块：

- `lib/osal_errno.c` - 错误码处理
- `lib/osal_heap.c` - 内存管理
- `lib/osal_stdio.c` - 标准 I/O
- `lib/osal_string.c` - 字符串操作
- `util/osal_log.c` - 日志功能
- `util/osal_version.c` - 版本信息

### 步骤 3: 更新 Kconfig

在 `Kconfig` 中添加新的 OS 选项：

```kconfig
config OS_NEWOS
    bool "NewOS"
    help
      Support for NewOS platform.
```

在 `core/osal/Kconfig` 中添加：

```kconfig
config OSAL_OS_NEWOS
    bool
    default y if OS_NEWOS
    help
      Use NewOS implementation for OSAL.
```

### 步骤 4: 更新 Makefile

在 `core/osal/Makefile` 中添加：

```makefile
ifeq ($(CONFIG_OSAL_OS_NEWOS),y)
  OSAL_OS_DIR := newos
endif

ifeq ($(CONFIG_OSAL_OS_NEWOS),y)
  ccflags-y += -DOSAL_OS_NEWOS
endif
```

### 步骤 5: 更新平台检测

在 `core/osal/include/osal_platform.h` 中添加：

```c
#elif defined(OSAL_OS_NEWOS)
    #define OSAL_PLATFORM_NEWOS
```

## 测试

### 测试不同平台配置

```bash
# 测试 ARM64 + RTOS
make defconfig
# 修改 .config: CONFIG_OS_RTOS=y, CONFIG_ARCH_ARM64=y
make syncconfig
make core/osal/

# 测试 ARM32 + Bare Metal
make defconfig
# 修改 .config: CONFIG_OS_BARE=y, CONFIG_ARCH_ARM32=y
make syncconfig
make core/osal/
```

### 验证源码目录选择

```bash
make core/osal/ V=1 | grep "src/.*/lib/osal_errno.o"
```

应该看到正确的源码目录（posix/win32/rtos/bare）。

## 常见问题

### Q: 如何查看当前的平台配置？

```bash
grep -E "CONFIG_OS=|CONFIG_ARCH=" .config
grep -E "CONFIG_OSAL_OS_|CONFIG_OSAL_ARCH_" .config
```

### Q: 为什么编译时找不到源文件？

检查对应的源码目录是否存在：

```bash
ls -la core/osal/src/posix/  # 或 win32/rtos/bare
```

如果目录不存在，说明该平台尚未实现，需要先移植。

### Q: 如何添加平台特定的编译标志？

在 `core/osal/Makefile` 中添加：

```makefile
ifeq ($(CONFIG_OSAL_OS_RTOS),y)
  ccflags-y += -DRTOS_SPECIFIC_FLAG
  osal-ldflags += -lrtos_lib
endif
```

### Q: 如何在代码中检测平台？

使用 `osal_platform.h` 中的宏：

```c
#include <osal_platform.h>

#if defined(OSAL_PLATFORM_POSIX)
    // POSIX 特定代码
#elif defined(OSAL_PLATFORM_WINDOWS)
    // Windows 特定代码
#elif defined(OSAL_PLATFORM_RTOS)
    // RTOS 特定代码
#endif
```

## 参考

- [PLATFORM.md](PLATFORM.md) - 平台配置总体说明
- [KCONFIG.md](KCONFIG.md) - Kconfig 配置系统
- [BUILD.md](BUILD.md) - 构建系统详解
