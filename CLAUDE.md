# EMS 项目指南 - Claude AI 助手

本文档为 AI 助手提供 EMS 项目的完整上下文，帮助理解项目架构、构建系统和开发规范。

## 项目概述

EMS (Embedded Management System) 是一个采用 Linux Kbuild 构建框架的嵌入式软件项目。

### 核心特点

- **构建系统**: Linux 内核风格的 Kbuild 框架
- **配置管理**: Kconfig 配置系统（menuconfig）
- **架构模式**: Core/Products 分层架构
- **编程语言**: C 语言（ISO C90 标准）
- **目标平台**: Linux 嵌入式系统（主要是 TI AM62x）

## 项目架构

### 目录结构

```
EMS/
├── core/                   # 核心模块（可复用组件）
│   ├── acl/               # 访问控制层
│   ├── hal/               # 硬件抽象层（CAN、UART、I2C、SPI、GPIO 等）
│   ├── osal/              # 操作系统抽象层（线程、互斥锁、信号量等）
│   ├── pcl/               # 协议控制层
│   └── pdl/               # 协议数据层
├── products/              # 产品应用（特定产品的实现）
│   └── ccm/              # CCM 产品
│       ├── apps/         # 应用程序（collector、logger、health、supervisor、comm）
│       └── libs/         # 产品库（libccm）
├── configs/               # Kconfig 配置文件（defconfig）
├── scripts/               # 构建脚本
│   ├── Makefile.build    # 核心构建规则（应用、库、模块）
│   ├── Makefile.clean    # 清理规则
│   ├── Makefile.lib      # 库函数
│   └── kconfig/          # Kconfig 配置工具
├── include/               # Staging 头文件目录（构建时自动生成）
├── bin/                   # 可执行文件输出目录
├── lib/                   # 库文件输出目录
│   └── modules/          # 内核模块输出目录
└── docs/                  # 项目文档
```

### 模块依赖关系

```
products/ccm/apps/*  →  products/ccm/libs/libccm  →  core/osal
                                                   →  core/hal
                                                   →  core/acl
                                                   →  core/pcl
                                                   →  core/pdl
```

**重要规则**:
- Products 依赖 Core，但 Core 不能依赖 Products
- 应用程序必须在库构建完成后才能构建
- 在顶层 Makefile 中通过 `products/: core/` 声明依赖

## 构建系统详解

### 平台配置

EMS 支持多架构和多操作系统配置，类似 Linux 内核的配置方式。

#### 架构配置 (ARCH)

支持的架构：
- `x86_64`: 64位 Intel/AMD 处理器
- `arm`: ARM32 (ARMv7-A)
- `arm64`: ARM64 (ARMv8-A / AArch64)
- `riscv`: RISC-V 64位

配置方式：
```bash
# 方法 1: Kconfig 配置（推荐）
make menuconfig
# 进入 "Target Platform" -> "Target architecture"

# 方法 2: 环境变量（临时覆盖）
make ARCH=arm64
export ARCH=arm64
```

#### 操作系统配置 (OS)

支持的操作系统：
- `linux`: Linux 操作系统（POSIX API）
- `windows`: Windows 操作系统（Win32 API）
- `rtos`: 实时操作系统（FreeRTOS、RT-Thread 等）
- `macos`: macOS 操作系统（POSIX + macOS 扩展）
- `bare`: 裸机环境（无操作系统）

配置方式：
```bash
# 方法 1: Kconfig 配置（推荐）
make menuconfig
# 进入 "Target Platform" -> "Target operating system"

# 方法 2: 环境变量（临时覆盖）
make OS=rtos
export OS=rtos
```

#### 交叉编译配置 (CROSS_COMPILE)

配置方式：
```bash
# 方法 1: Kconfig 配置（推荐）
make menuconfig
# 进入 "Target Platform" -> "Cross-compiler prefix"

# 方法 2: 环境变量（临时覆盖）
make CROSS_COMPILE=aarch64-linux-gnu-
export CROSS_COMPILE=aarch64-linux-gnu-
```

默认工具链前缀：
- x86_64: (空，本地编译)
- arm: `arm-linux-gnueabihf-`
- arm64: `aarch64-linux-gnu-`
- riscv: `riscv64-linux-gnu-`

#### 常见配置场景

```bash
# 本地 x86_64 Linux 开发
make ARCH=x86_64 OS=linux

# ARM64 Linux 交叉编译
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- OS=linux

# ARM32 RTOS 开发
make ARCH=arm CROSS_COMPILE=arm-none-eabi- OS=rtos

# RISC-V 裸机开发
make ARCH=riscv CROSS_COMPILE=riscv64-unknown-elf- OS=bare
```

详细说明请参考 [docs/PLATFORM.md](docs/PLATFORM.md)。

### OSAL 平台适配

OSAL (Operating System Abstraction Layer) 根据 OS 和 ARCH 配置自动选择对应的源码实现。

#### 支持的平台

| OS 类型 | 源码目录 | 说明 |
|---------|----------|------|
| Linux/macOS | `src/posix/` | POSIX 兼容系统 |
| Windows | `src/win32/` | Win32 API |
| RTOS | `src/rtos/` | 实时操作系统 |
| Bare Metal | `src/bare/` | 裸机环境 |

| 架构 | 位宽 | 说明 |
|------|------|------|
| x86_64 | 64-bit | Intel/AMD 64位 |
| ARM32 | 32-bit | ARM 32位 |
| ARM64 | 64-bit | ARM 64位 (AArch64) |
| RISC-V 64 | 64-bit | RISC-V 64位 |

#### 配置示例

```bash
# ARM64 + RTOS
make menuconfig
# 选择: Target Platform -> Target operating system -> RTOS
# 选择: Target Platform -> Target architecture -> ARM64 (AArch64)
make core/osal/

# 验证源码目录选择
make core/osal/ V=1 | grep "src/.*/lib/osal_errno.o"
# 应该显示: src/rtos/lib/osal_errno.o
```

#### 自动配置

根据 OS 和 ARCH 配置，OSAL 会自动设置：

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

详细说明请参考 [docs/OSAL_PLATFORM.md](docs/OSAL_PLATFORM.md)。

### Kbuild 框架核心概念

#### 1. 声明式 Makefile

使用声明式变量而非命令式规则：

```makefile
# ✅ 正确：声明式
app-y := ccm_collector
lib-y := osal
so-y := libccm
obj-m := driver

# ❌ 错误：不要写命令式规则
ccm_collector: main.o
	$(CC) -o $@ $^
```

#### 2. 复合对象

使用 `xxx-objs` 定义多文件目标：

```makefile
app-y := myapp
myapp-objs := main.o config.o logger.o utils.o

lib-y := mylib
mylib-objs := api.o internal.o platform.o
```

#### 3. 条件编译

使用 Kconfig 变量控制编译：

```makefile
# 条件编译源文件
obj-$(CONFIG_OSAL_NETWORK) += osal_socket.o
obj-$(CONFIG_HAL_CAN) += hal_can_linux.o

# 条件编译子目录
subdir-$(CONFIG_CCM_APPS) += apps/

# 条件编译库
lib-$(CONFIG_BUILD_SHARED) := mylib
```

#### 4. Staging 头文件机制

**核心特性**: 库的头文件自动安装到 `$(objtree)/include/`，应用程序无需知道库的源码路径。

**库 Makefile**:
```makefile
lib-y := osal
osal-objs := osal_thread.o osal_mutex.o

# 声明需要导出的头文件（相对于 $(src)/include/）
header-y := osal.h osal_types.h
header-y += ipc/osal_mutex.h ipc/osal_semaphore.h
header-y += sys/osal_thread.h

ccflags-y += -I$(src)/include
```

**头文件目录结构**:
```
core/osal/
├── include/           # 头文件源码目录
│   ├── osal.h
│   ├── osal_types.h
│   ├── ipc/
│   │   ├── osal_mutex.h
│   │   └── osal_semaphore.h
│   └── sys/
│       └── osal_thread.h
├── src/
└── Makefile
```

**应用程序 Makefile**:
```makefile
app-y := myapp
myapp-objs := main.o

# 只需包含 staging 目录，自动获得所有库头文件
ccflags-y += -I$(objtree)/include
ccflags-y += -I$(src)/include
```

**实现位置**: `scripts/Makefile.build:471-503`

#### 5. 智能库名处理

自动检测库名前缀，避免重复：

```makefile
lib-y += osal        # → libosal.a
lib-y += libosal.a   # → libosal.a（不会变成 liblibosal.a）

so-y += ccm          # → libccm.so
so-y += libccm.so    # → libccm.so
```

**实现位置**: `scripts/Makefile.build:276-278, 329-331`

### 构建流程

```
1. 配置阶段
   make menuconfig / make xxx_defconfig
   ↓
   生成 .config 和 include/config/auto.conf

2. 准备阶段
   创建输出目录（bin/、lib/、lib/modules/）
   ↓
   构建 kconfig 工具

3. 构建阶段
   递归进入子目录（core/、products/）
   ↓
   编译源文件 (.c → .o)
   ↓
   链接库文件 (.o → .a / .so)
   ↓
   安装头文件到 staging 目录
   ↓
   链接应用程序 (.o → bin/)

4. 安装阶段
   复制文件到 bin/、lib/、lib/modules/
```

### 关键文件说明

#### Makefile (顶层)

- 定义全局变量（CC、CFLAGS、输出目录等）
- 实现配置目标（menuconfig、defconfig 等）
- 定义清理目标（clean、mrproper、distclean）
- 控制递归构建流程

**重要变量**:
```makefile
srctree := .                    # 源码根目录
objtree := .                    # 输出根目录（支持 O=dir）
BIN_DIR := $(objtree)/bin       # 可执行文件目录
LIB_DIR := $(objtree)/lib       # 库文件目录
MODULES_DIR := $(objtree)/lib/modules  # 内核模块目录

core-y := core                  # 核心模块目录
products-y := products          # 产品模块目录
```

#### scripts/Makefile.build

核心构建规则文件，处理：
- 应用程序构建（app-y）
- 静态库构建（lib-y）
- 动态库构建（so-y）
- 内核模块构建（obj-m）
- 头文件安装（header-y）
- 依赖跟踪（.cmd 文件）

**关键规则**:
- 第 276-278 行：静态库安装（智能前缀处理）
- 第 329-331 行：动态库安装（智能前缀处理）
- 第 471-503 行：Staging 头文件自动安装

#### scripts/Makefile.clean

清理规则文件，支持：
- 递归清理子目录
- 清理编译产物（.o、.a、.so、.cmd）
- 清理 kconfig 工具（hostprogs、targets）
- 清理 staging 头文件

**清理变量**:
```makefile
clean-files := file1 file2      # 需要清理的文件
clean-dirs := dir1 dir2         # 需要清理的目录
hostprogs := conf mconf         # 主机程序（自动清理）
targets := parser.tab.c         # 生成的目标文件（自动清理）
subdir- := kconfig              # 需要递归清理的子目录
```

#### scripts/kconfig/Makefile

Kconfig 工具构建规则：
- 构建 conf（命令行配置工具）
- 构建 mconf（menuconfig 图形界面）
- 生成词法/语法分析器（lexer.lex.c、parser.tab.c）

### 常用构建命令

```bash
# 配置
make menuconfig                        # 图形化配置
make ccm_h200_am625_debug_defconfig   # 加载预定义配置
make savedefconfig                     # 保存最小化配置
make PRODUCT=ccm_h200_am625_debug     # 使用 PRODUCT 变量

# 编译
make                                   # 编译所有目标
make -j$(nproc)                       # 并行编译
make V=1                              # 详细输出
make core                             # 只编译 core
make products                         # 只编译 products

# 安装
make install                          # 安装所有文件到 /usr/local
make install prefix=/usr              # 安装到 /usr
make install DESTDIR=/tmp/install     # 安装到临时目录（用于打包）
make install_bin                      # 只安装可执行文件
make install_lib                      # 只安装库文件
make install_headers                  # 只安装头文件

# 清理
make clean                            # 清理编译产物
make mrproper                         # 深度清理（包括配置）
make distclean                        # 完全清理

# 调试
make -n                               # 显示命令但不执行
make --debug=v                        # 调试 Makefile
```

## 编码规范

### C 语言规范

- **标准**: ISO C90（不使用 C99/C11 特性）
- **风格**: Linux 内核编码风格
- **缩进**: Tab（8 空格宽度）
- **命名**: 小写加下划线（snake_case）

### 常见问题

#### ❌ 混合声明和代码

```c
// 错误：C90 不允许
void func(void) {
    int a = 1;
    printf("%d\n", a);
    int b = 2;  // ❌ 声明必须在代码块开头
}

// 正确
void func(void) {
    int a = 1;
    int b = 2;
    printf("%d\n", a);
}
```

#### ❌ 使用 C99 特性

```c
// 错误：for 循环中声明变量
for (int i = 0; i < 10; i++) { }  // ❌

// 正确
int i;
for (i = 0; i < 10; i++) { }
```

### Makefile 规范

```makefile
# 1. 使用 Tab 缩进（不是空格）
# 2. 变量赋值使用 := 或 +=
# 3. 条件编译使用 obj-$(CONFIG_XXX)
# 4. 头文件路径使用 ccflags-y
# 5. 链接选项使用 ldflags-y

# 示例
app-y := myapp
myapp-objs := main.o utils.o

ccflags-y += -I$(objtree)/include
ccflags-y += -I$(src)/include
ccflags-$(CONFIG_DEBUG) += -DDEBUG

ldflags-y += -lpthread
```

## 常见开发任务

### 添加新的应用程序

1. 创建目录：`products/ccm/apps/myapp/`
2. 创建 Makefile：
```makefile
app-y := myapp
myapp-objs := main.o module1.o module2.o

ccflags-y += -I$(objtree)/include
ccflags-y += -I$(src)/include
```
3. 在父目录 Makefile 添加：`subdir-y += myapp`
4. 编译测试：`make products`

### 添加新的库

1. 创建目录：`core/mylib/`
2. 创建头文件目录：`core/mylib/include/`
3. 创建 Makefile：
```makefile
lib-y := mylib
mylib-objs := api.o internal.o

# 导出头文件
header-y := mylib.h mylib_types.h
header-y += subdir/mylib_api.h

ccflags-y += -I$(src)/include
```
4. 在父目录 Makefile 添加：`subdir-y += mylib`
5. 编译测试：`make core`

### 添加 Kconfig 选项

1. 编辑相应的 Kconfig 文件（如 `core/osal/Kconfig`）
2. 添加配置项：
```kconfig
config OSAL_NEW_FEATURE
    bool "Enable new OSAL feature"
    default y
    help
      This enables the new OSAL feature.
```
3. 在 Makefile 中使用：
```makefile
obj-$(CONFIG_OSAL_NEW_FEATURE) += new_feature.o
```
4. 运行 `make menuconfig` 验证

### 调试构建问题

```bash
# 1. 查看详细构建过程
make V=1

# 2. 查看 Make 变量
make -p | grep "^app-y"

# 3. 检查依赖文件
cat core/osal/.osal_thread.o.cmd

# 4. 清理后重新构建
make mrproper
make ccm_h200_am625_debug_defconfig
make -j$(nproc)

# 5. 检查配置
cat .config | grep CONFIG_OSAL
```

## 重要注意事项

### 构建系统

1. **不要修改 scripts/ 下的核心文件**，除非你完全理解 Kbuild 机制
2. **始终使用声明式语法**，不要在子目录 Makefile 中写命令式规则
3. **头文件必须放在 include/ 子目录**，并通过 header-y 声明
4. **应用程序只能使用 staging 头文件**，不要硬编码源码路径
5. **库名可以带或不带 lib 前缀**，构建系统会自动处理

### 依赖关系

1. **Products 必须依赖 Core**，在顶层 Makefile 中声明 `products/: core/`
2. **应用程序必须在库之后构建**，通过目录顺序控制
3. **不要创建循环依赖**，Core 模块之间也要注意依赖顺序

### 清理机制

1. **make clean**: 清理编译产物，保留配置
2. **make mrproper**: 深度清理，删除配置和 kconfig 工具
3. **make distclean**: 完全清理，包括编辑器备份文件

### Git 工作流

1. **当前分支**: `feature/kconfig-integration`
2. **主分支**: `master`
3. **提交前检查**: 确保 `make mrproper && make ccm_h200_am625_debug_defconfig && make -j$(nproc)` 成功

## 最近的改进

### 2026-05-21 改进记录

1. **Staging 头文件机制** (High Priority)
   - 自动安装库头文件到 `$(objtree)/include/`
   - 简化应用程序 Makefile（减少 35 行硬编码路径）
   - 支持外部构建和 Buildroot/Yocto 集成

2. **库名前缀智能处理** (Medium Priority)
   - 避免 `lib-y += libosal.a` 变成 `liblibosal.a`
   - 自动检测并保持正确的库名

3. **PRODUCT 变量支持** (Low Priority)
   - `make PRODUCT=ccm` 自动加载配置
   - 简化 CI/CD 流程

4. **Kconfig 清理机制优化**
   - 参考 Linux 内核实现
   - 支持 hostprogs、targets、subdir- 变量
   - 正确清理 kconfig 工具和生成文件

5. **目录结构标准化**
   - 内核模块从 `ko/` 移到 `lib/modules/`
   - 符合 Linux 标准目录布局

6. **清理输出格式修复**
   - 修复双斜杠问题（`core//acl` → `core/acl`）
   - 消除重复目标定义警告

详见 `IMPROVEMENTS.md`。

## 参考文档

- [构建系统详解](docs/BUILD_SYSTEM.md)
- [构建指南](docs/BUILD_GUIDE.md)
- [架构设计](docs/ARCHITECTURE.md)
- [Kconfig 集成](docs/KCONFIG_INTEGRATION_SUMMARY.md)
- [Makefile 框架](docs/MAKEFILE_FRAMEWORK.md)

## 故障排除快速参考

| 问题 | 解决方案 |
|------|---------|
| 找不到头文件 | 检查 header-y 声明和 ccflags-y 设置 |
| 库名重复前缀 | 已修复，可以使用任意格式 |
| 清理不完全 | 使用 `make mrproper` |
| 配置不生效 | 重新运行 `make menuconfig` 并保存 |
| 并行构建失败 | 检查依赖关系声明 |
| 双斜杠路径 | 已修复，确保使用最新代码 |

---

**最后更新**: 2026-05-21
**维护者**: wanguo
**分支**: feature/kconfig-integration
