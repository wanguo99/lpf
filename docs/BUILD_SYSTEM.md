# EMS 构建系统文档

## 概述

EMS 项目采用 Linux 内核风格的 Kbuild 构建系统，支持：
- **Kconfig 配置管理** - menuconfig 图形化配置界面
- **条件编译** - `obj-$(CONFIG_XXX)` 语法
- **声明式 Makefile** - app-y、lib-y、so-y、obj-m 变量
- **Staging 头文件机制** - 自动安装库头文件
- **智能库名处理** - 自动检测 lib 前缀
- **递归子目录构建** - 自动处理依赖关系
- **依赖跟踪** - .cmd 文件跟踪命令行变化
- **静默编译输出** - 简洁的构建信息
- **外部构建支持** - O=dir 指定输出目录
- **并行编译** - make -j 支持

## 核心构建变量

### 声明式语法

EMS 使用声明式变量定义构建目标，而不是命令式规则：

| 变量 | 用途 | 示例 |
|------|------|------|
| `app-y` | 可执行程序 | `app-y := ccm_collector` |
| `lib-y` | 静态库 | `lib-y := osal` |
| `so-y` | 动态库 | `so-y := ccm` |
| `obj-m` | 内核模块 | `obj-m := driver` |
| `obj-y` | 目标文件（用于复合对象） | `obj-y := main.o utils.o` |
| `subdir-y` | 子目录 | `subdir-y += apps/` |
| `header-y` | 导出头文件 | `header-y := osal.h` |

### 复合对象

使用 `xxx-objs` 定义多文件目标：

```makefile
# 应用程序
app-y := ccm_collector
ccm_collector-objs := main.o config.o logger.o utils.o

# 静态库
lib-y := osal
osal-objs := osal_thread.o osal_mutex.o osal_semaphore.o

# 动态库
so-y := ccm
ccm-objs := api.o internal.o platform.o
```

### 编译选项

| 变量 | 用途 | 示例 |
|------|------|------|
| `ccflags-y` | C 编译标志 | `ccflags-y += -I$(objtree)/include` |
| `ldflags-y` | 链接标志 | `ldflags-y += -lpthread` |
| `asflags-y` | 汇编标志 | `asflags-y += -D__ASSEMBLY__` |

## 条件编译机制

### obj-$(CONFIG_XXX) 语法

参考 Linux 内核，EMS 支持使用 `obj-$(CONFIG_XXX)` 语法进行条件编译：

```makefile
# 条件编译源文件
obj-$(CONFIG_OSAL_NETWORK) += osal_socket.o
obj-$(CONFIG_OSAL_NETWORK) += osal_termios.o

# 条件编译子目录
subdir-$(CONFIG_CCM_APPS) += apps/

# 条件编译库
lib-$(CONFIG_BUILD_SHARED) := mylib
```

当 `CONFIG_OSAL_NETWORK=y` 时，这些文件会被编译；当配置为 `n` 时，这些文件会被跳过。

### 自动展开机制

构建系统会自动展开 `obj-$(CONFIG_XXX)` 变量：

1. Kconfig 生成 `include/config/auto.conf`，包含所有配置变量
2. `scripts/Makefile.build` 包含 auto.conf，展开条件变量
3. 支持 `obj-$(CONFIG_XXX)`、`lib-$(CONFIG_XXX)`、`so-$(CONFIG_XXX)`、`app-$(CONFIG_XXX)`、`subdir-$(CONFIG_XXX)`

### 使用示例

#### OSAL 子功能条件编译

```makefile
# core/osal/Makefile
obj-y += osal_init.o                     # 始终编译
obj-$(CONFIG_OSAL_FILE) += osal_file.o   # 条件编译
obj-$(CONFIG_OSAL_SIGNAL) += osal_signal.o
obj-$(CONFIG_OSAL_THREAD) += osal_thread.o
obj-$(CONFIG_OSAL_NETWORK) += osal_socket.o
```

#### HAL 驱动条件编译

```makefile
# core/hal/Makefile
obj-$(CONFIG_HAL_CAN) += hal_can_linux.o
obj-$(CONFIG_HAL_UART) += hal_serial_linux.o
obj-$(CONFIG_HAL_I2C) += hal_i2c_linux.o
obj-$(CONFIG_HAL_SPI) += hal_spi_linux.o
obj-$(CONFIG_HAL_GPIO) += hal_gpio_linux.o
```

#### 应用程序条件编译

```makefile
# products/ccm/apps/Makefile
subdir-$(CONFIG_CCM_COLLECTOR) += ccm_collector/
subdir-$(CONFIG_CCM_LOGGER) += ccm_logger/
subdir-$(CONFIG_CCM_HEALTH) += ccm_health/
```

## Staging 头文件机制

### 概述

Staging 机制是 EMS 构建系统的核心特性之一，它自动将库的头文件安装到 `$(objtree)/include/` 目录，使得应用程序无需知道库的源码路径。

### 工作原理

1. **库 Makefile 声明导出头文件**：
```makefile
# core/osal/Makefile
lib-y := osal
osal-objs := osal_thread.o osal_mutex.o

# 声明需要导出的头文件（相对于 $(src)/include/）
header-y := osal.h osal_types.h
header-y += ipc/osal_mutex.h ipc/osal_semaphore.h
header-y += sys/osal_thread.h

ccflags-y += -I$(src)/include
```

2. **构建时自动安装**：
   - `scripts/Makefile.build` 在构建库时自动复制 `header-y` 中的文件
   - 从 `$(src)/include/` 复制到 `$(objtree)/include/`
   - 保持目录结构（如 `ipc/osal_mutex.h` → `include/ipc/osal_mutex.h`）

3. **应用程序使用 staging 头文件**：
```makefile
# products/ccm/apps/ccm_collector/Makefile
app-y := ccm_collector
ccm_collector-objs := main.o collector.o

# 头文件路径自动包含（通过 EMSINCLUDE）
# 无需手动添加 -I$(INCLUDE_DIR)
```

### 优势

- **封装性好**：应用程序不需要知道库的源码路径
- **简化 Makefile**：无需手动添加 `-I` 路径，自动包含 staging 头文件
- **支持外部构建**：`O=dir` 时头文件自动安装到正确位置
- **便于集成**：Buildroot/Yocto 可以通过 `STAGING_DIR` 环境变量直接指定路径
- **灵活配置**：支持三级优先级（命令行 > 环境变量 > 默认值）

### 头文件目录结构

```
core/osal/
├── include/              # 头文件源码目录
│   ├── osal.h
│   ├── osal_types.h
│   ├── ipc/
│   │   ├── osal_mutex.h
│   │   └── osal_semaphore.h
│   └── sys/
│       └── osal_thread.h
├── src/                  # 源文件目录
└── Makefile

构建后：
$(STAGING_DIR)/include/   # Staging 目录（默认 .staging/include）
├── osal.h
├── osal_types.h
├── ipc/
│   ├── osal_mutex.h
│   └── osal_semaphore.h
├── sys/
│   └── osal_thread.h
└── config/               # Kconfig 生成的配置头文件
    └── ems_config.h
```

### Staging 目录配置

**默认配置**：
```makefile
# Makefile
STAGING_DIR ?= $(objtree)/.staging
INCLUDE_DIR := $(STAGING_DIR)/include
EMSINCLUDE  := -I$(INCLUDE_DIR)
```

**环境变量配置**：
```bash
# 使用自定义路径
export STAGING_DIR=/tmp/ems-staging
make

# 或一次性指定
STAGING_DIR=/tmp/ems-staging make
```

**Buildroot 集成**：
```makefile
# ems.mk
EMS_MAKE_ENV = STAGING_DIR=$(STAGING_DIR)

define EMS_BUILD_CMDS
    $(TARGET_MAKE_ENV) $(EMS_MAKE_ENV) $(MAKE) -C $(@D)
endef
```

**配置优先级**：
```
命令行参数 > 环境变量 > 默认值 (.staging)
```

### 实现位置

**核心逻辑**：`scripts/Makefile.build:480-505`

**关键代码**：
- 第 486-493 行：头文件安装规则
  ```makefile
  define install-header-rule
  __install_hdr_$(subst /,_,$(1)): FORCE
      @mkdir -p $$(dir $(INCLUDE_DIR)/$(1))
      @if [ -f $(src)/include/$(1) ]; then \
          cp -f $(src)/include/$(1) $(INCLUDE_DIR)/$(1); \
          echo "  INSTALL include/$(1)"; \
      fi
  endef
  ```

- 第 501-503 行：依赖关系（库构建后自动安装头文件）
  ```makefile
  ifneq ($(lib-target)$(so-target),)
  $(lib-target) $(so-target): __install_headers
  endif
  ```

**清理规则**：`Makefile:CLEAN_DIRS` 包含 `include/` 子目录

## 智能库名处理

### 问题背景

在早期版本中，如果用户写 `lib-y += libosal.a`，构建系统会自动添加 `lib` 前缀，导致最终生成 `liblibosal.a`。

### 解决方案

构建系统现在会智能检测库名是否已包含 `lib` 前缀：

```makefile
# 两种写法都正确
lib-y += osal        # → libosal.a
lib-y += libosal.a   # → libosal.a（不会变成 liblibosal.a）

so-y += ccm          # → libccm.so
so-y += libccm.so    # → libccm.so
```

### 实现位置

**静态库**：`scripts/Makefile.build:276-279`

**实现逻辑**：
```makefile
define get-lib-install-name
$(if $(filter lib%,$(notdir $(1))),$(notdir $(1)),lib$(notdir $(1)))
endef
```

检测库名是否已有 `lib` 前缀：
- 如果库名以 `lib` 开头，直接使用原名
- 如果库名不以 `lib` 开头，自动添加 `lib` 前缀

**动态库**：`scripts/Makefile.build:329-331`（类似的 `get-so-install-name` 函数）

## Makefile 编写规范

### 应用程序 Makefile

```makefile
# products/ccm/apps/ccm_collector/Makefile

# 声明应用程序
app-y := ccm_collector

# 定义源文件
ccm_collector-objs := main.o collector.o config.o logger.o

# 编译选项
ccflags-y += -I$(objtree)/include    # Staging 头文件
ccflags-y += -I$(src)/include        # 本地头文件
ccflags-y += -Wall -Werror

# 链接选项
ldflags-y += -lpthread -lrt
```

### 静态库 Makefile

```makefile
# core/osal/Makefile

# 声明静态库
lib-y := osal

# 定义源文件
osal-objs := osal_thread.o osal_mutex.o osal_semaphore.o
osal-objs += osal_queue.o osal_timer.o

# 导出头文件
header-y := osal.h osal_types.h
header-y += ipc/osal_mutex.h ipc/osal_semaphore.h
header-y += sys/osal_thread.h

# 编译选项
ccflags-y += -I$(src)/include
ccflags-y += -Wall -Werror

# 条件编译
obj-$(CONFIG_OSAL_NETWORK) += osal_socket.o
```

### 动态库 Makefile

```makefile
# products/ccm/libs/libccm/Makefile

# 声明动态库
so-y := ccm

# 定义源文件
ccm-objs := api.o internal.o platform.o

# 导出头文件
header-y := ccm.h ccm_types.h

# 编译选项
ccflags-y += -I$(objtree)/include
ccflags-y += -I$(src)/include
ccflags-y += -fPIC

# 链接选项
ldflags-y += -losal -lhal
```

### 内核模块 Makefile

```makefile
# drivers/mydriver/Makefile

# 声明内核模块
obj-m := mydriver.o

# 定义源文件
mydriver-objs := main.o device.o interrupt.o

# 编译选项
ccflags-y += -I$(src)/include
```

### 子目录 Makefile

```makefile
# products/ccm/apps/Makefile

# 递归进入子目录
subdir-y += ccm_collector/
subdir-y += ccm_logger/
subdir-$(CONFIG_CCM_HEALTH) += ccm_health/
subdir-$(CONFIG_CCM_SUPERVISOR) += ccm_supervisor/
```

## 输出目录结构

```
$(objtree)/
├── bin/                  # 可执行文件
│   ├── ccm_collector
│   ├── ccm_logger
│   └── ccm_health
├── lib/                  # 库文件
│   ├── libosal.a
│   ├── libhal.a
│   ├── libccm.so
│   └── modules/         # 内核模块
│       └── mydriver.ko
└── include/             # Staging 头文件
    ├── osal.h
    ├── hal.h
    └── ccm.h
```

## 配置管理

### 配置文件

| 文件 | 用途 |
|------|------|
| `.config` | 用户配置文件（手动编辑或 menuconfig 生成） |
| `include/config/auto.conf` | 自动生成的 Makefile 配置（供构建系统使用） |
| `include/generated/autoconf.h` | 自动生成的 C 头文件（供源码使用） |
| `configs/*_defconfig` | 预定义配置文件 |

### 配置命令

```bash
# 图形化配置
make menuconfig

# 使用预定义配置
make ccm_h200_am625_debug_defconfig

# 使用 PRODUCT 变量（自动加载 configs/$(PRODUCT)_defconfig）
make PRODUCT=ccm_h200_am625_debug

# 保存最小化配置
make savedefconfig

# 同步配置（.config → auto.conf）
make syncconfig

# 使用默认配置
make defconfig

# 旧配置升级（添加新选项）
make olddefconfig
```

### 配置变更流程

1. 修改配置：
   ```bash
   make menuconfig
   # 或
   vim .config
   ```

2. 同步配置（自动执行，也可手动）：
   ```bash
   make syncconfig
   ```

3. 重新编译：
   ```bash
   make clean && make
   ```

## 构建目标

### 完整构建

```bash
# 编译所有目标
make

# 并行编译（推荐）
make -j$(nproc)

# 详细输出
make V=1

# 静默输出
make V=0
```

### 核心模块

```bash
make core          # 编译所有核心模块
make core/osal     # 只编译 OSAL
make core/hal      # 只编译 HAL
make core/pcl      # 只编译 PCL
make core/pdl      # 只编译 PDL
make core/acl      # 只编译 ACL
```

### 产品模块

```bash
make products      # 编译所有产品
make products/ccm  # 只编译 CCM 产品
```

### 清理目标

```bash
# 清理编译产物（保留配置）
make clean
# 删除：bin/、lib/、include/（staging 头文件）、*.o、*.a、*.so、.*.cmd

# 深度清理（删除配置和 kconfig 工具）
make mrproper
# 额外删除：.config、include/config/、scripts/kconfig/conf、scripts/kconfig/mconf

# 完全清理（包括编辑器备份文件）
make distclean
# 额外删除：*~、*.swp、*.bak
```

### 调试目标

```bash
# 显示命令但不执行
make -n

# 显示 Make 调试信息
make --debug=v

# 显示变量值
make -p | grep "^app-y"
```

## 依赖跟踪

构建系统自动跟踪头文件依赖和命令行变化。

**实现位置**：
- 依赖生成：`scripts/Kbuild.include:217` (`cmd_and_fixdep` 函数)
- 依赖读取：`Makefile:699-704`
- 命令比较：`scripts/Kbuild.include:209-239` (`if_changed` 系列函数)

### .d 依赖文件

- 使用 `-MMD -MP` 编译标志生成 `.d` 文件
- 自动包含 `.d` 文件以跟踪头文件依赖关系
- 头文件修改后自动重新编译相关源文件

示例：
```makefile
# core/osal/.osal_thread.o.d
osal_thread.o: osal_thread.c \
  include/osal.h \
  include/osal_types.h \
  include/sys/osal_thread.h
```

### .cmd 命令文件

- 记录每个目标的完整编译命令
- 命令行变化时自动重新编译
- 实现 `if_changed` 机制

示例：
```makefile
# core/osal/.osal_thread.o.cmd
cmd_core/osal/osal_thread.o := gcc -Wp,-MMD,core/osal/.osal_thread.o.d \
  -Wall -Werror -I./include -c -o core/osal/osal_thread.o core/osal/osal_thread.c
```

### if_changed 机制

只有在以下情况才重新构建：
1. 源文件或头文件被修改
2. 编译命令行发生变化
3. 目标文件不存在

这避免了不必要的重新编译，提高构建效率。

## 静默编译

默认使用静默编译输出，只显示简洁的编译信息：

```
  CC      core/osal/osal_thread.o
  CC      core/osal/osal_mutex.o
  AR      lib/libosal.a
  CC      core/hal/hal_can_linux.o
  AR      lib/libhal.a
  CC      products/ccm/apps/ccm_collector/main.o
  LD      bin/ccm_collector
```

### 输出格式

| 前缀 | 含义 |
|------|------|
| `CC` | 编译 C 源文件 |
| `AR` | 创建静态库 |
| `LD` | 链接可执行文件或动态库 |
| `INSTALL` | 安装文件到输出目录 |
| `HOSTCC` | 编译主机程序（如 kconfig 工具） |
| `CLEAN` | 清理目录 |

### 详细输出

使用 `V=1` 查看完整编译命令：

```bash
make V=1
```

输出示例：
```
gcc -Wp,-MMD,core/osal/.osal_thread.o.d -Wall -Werror -I./include \
  -c -o core/osal/osal_thread.o core/osal/osal_thread.c
ar rcs lib/libosal.a core/osal/osal_thread.o core/osal/osal_mutex.o
gcc -o bin/ccm_collector products/ccm/apps/ccm_collector/main.o \
  -L./lib -losal -lhal -lpthread
```

### 超详细输出

使用 `V=2` 查看 Make 内部调试信息：

```bash
make V=2
```

## 外部构建

支持在独立目录中构建，保持源码目录干净：

```bash
# 创建构建目录
mkdir build
cd build

# 指定源码目录和输出目录
make -C /path/to/EMS O=$(pwd) menuconfig
make -C /path/to/EMS O=$(pwd) -j$(nproc)

# 或使用相对路径
cd /path/to/EMS
make O=../build menuconfig
make O=../build -j$(nproc)
```

外部构建时：
- 所有编译产物（.o、.a、.so、bin/、lib/）都在 `O=` 指定的目录
- 源码目录保持干净，只有源文件
- Staging 头文件安装到 `$(O)/include/`
- 配置文件（.config）保存在 `$(O)/`

## 交叉编译

### 通过 Kconfig 配置

```bash
make menuconfig
# 导航到 "Build Options"
# 设置 "Cross-compiler tool prefix"
# 例如：aarch64-linux-gnu-
```

### 直接修改 .config

```bash
# 编辑 .config
CONFIG_CROSS_COMPILE="aarch64-linux-gnu-"

# 同步配置
make syncconfig
```

### 命令行指定

```bash
# 临时指定交叉编译器
make CROSS_COMPILE=aarch64-linux-gnu-

# 或直接指定编译器
make CC=aarch64-linux-gnu-gcc LD=aarch64-linux-gnu-ld
```

### 常用交叉编译器前缀

| 目标平台 | 前缀 |
|---------|------|
| ARM 32-bit | `arm-linux-gnueabihf-` |
| ARM 64-bit (AArch64) | `aarch64-linux-gnu-` |
| TI AM62x | `aarch64-none-linux-gnu-` |
| MIPS | `mips-linux-gnu-` |
| PowerPC | `powerpc-linux-gnu-` |

## 清理机制详解

### 清理级别

EMS 提供三个级别的清理目标：

#### 1. make clean

清理编译产物，保留配置：

**删除内容**：
- 所有目标文件（*.o）
- 所有库文件（*.a、*.so）
- 所有可执行文件（bin/）
- 所有依赖文件（*.d、.*.cmd）
- Staging 头文件（include/）
- 输出目录（lib/、lib/modules/）

**保留内容**：
- 配置文件（.config）
- Kconfig 工具（scripts/kconfig/conf、mconf）
- 源码文件

**使用场景**：重新编译但保持配置不变

#### 2. make mrproper

深度清理，删除配置和工具：

**额外删除**（在 clean 基础上）：
- 配置文件（.config、.config.old）
- 配置目录（include/config/、include/generated/）
- Kconfig 工具（scripts/kconfig/conf、mconf、lexer.lex.c、parser.tab.c）
- Kconfig 生成的头文件（scripts/kconfig/include/）

**使用场景**：切换配置或完全重新开始

#### 3. make distclean

完全清理，包括临时文件：

**额外删除**（在 mrproper 基础上）：
- 编辑器备份文件（*~、*.swp、*.bak）
- 补丁文件（*.orig、*.rej）
- 标签文件（tags、TAGS、cscope.*）

**使用场景**：准备发布或提交代码

### 清理实现

清理机制通过 `scripts/Makefile.clean` 实现，支持：

1. **递归清理子目录**：
```makefile
subdir-y := core products
subdir- += scripts/kconfig
```

2. **清理文件列表**：
```makefile
clean-files := conf mconf lexer.lex.c parser.tab.c
```

3. **清理目录列表**：
```makefile
clean-dirs := include bin lib
```

4. **自动清理变量**：
```makefile
# 自动清理 hostprogs、targets、lib-y、so-y、app-y
hostprogs := conf mconf
targets := parser.tab.c parser.tab.h
```

### 清理规则编写

在子目录 Makefile 中定义清理规则：

```makefile
# scripts/kconfig/Makefile

# 主机程序（自动清理）
hostprogs := conf mconf

# 生成的目标文件（自动清理）
targets := parser.tab.c parser.tab.h lexer.lex.c

# 额外需要清理的文件
clean-files := .config .config.old
clean-files += *.o lxdialog/*.o

# 需要清理的目录
clean-dirs := include

# 需要递归清理的子目录
subdir- := lxdialog
```

### 清理输出示例

```bash
$ make clean
  CLEAN   core/acl
  CLEAN   core/hal
  CLEAN   core/osal
  CLEAN   core/pcl
  CLEAN   core/pdl
  CLEAN   products/ccm/apps/ccm_collector
  CLEAN   products/ccm/apps/ccm_logger
  CLEAN   products/ccm/libs/libccm
  CLEAN   .

$ make mrproper
  CLEAN   scripts/kconfig
  CLEAN   include/config include/generated
  CLEAN   .config
```

## 最佳实践

### 1. 使用声明式语法

优先使用声明式变量而不是命令式规则：

```makefile
# ✅ 推荐：声明式
app-y := myapp
myapp-objs := main.o utils.o

lib-y := mylib
mylib-objs := api.o internal.o

# ❌ 不推荐：命令式
myapp: main.o utils.o
	$(CC) -o $@ $^

libmylib.a: api.o internal.o
	$(AR) rcs $@ $^
```

### 2. 使用条件编译

优先使用 `obj-$(CONFIG_XXX)` 而不是 `ifeq` 判断：

```makefile
# ✅ 推荐
obj-$(CONFIG_FEATURE) += feature.o
subdir-$(CONFIG_MODULE) += module/

# ❌ 不推荐
ifeq ($(CONFIG_FEATURE),y)
  obj-y += feature.o
endif
```

### 3. 使用 Staging 头文件

应用程序自动包含 staging 头文件，无需手动配置：

```makefile
# ✅ 推荐：无需手动添加头文件路径
app-y := myapp
myapp-objs := main.o utils.o
# EMSINCLUDE 已自动添加到编译命令

# ❌ 不推荐：硬编码库源码路径
ccflags-y += -I$(srctree)/core/osal/include
ccflags-y += -I$(srctree)/core/hal/include
ccflags-y += -I$(srctree)/core/acl/include
```

**说明**：
- `EMSINCLUDE` 变量在 `Makefile` 中定义为 `-I$(INCLUDE_DIR)`
- `scripts/Makefile.build` 自动将 `$(EMSINCLUDE)` 添加到 `c_flags`
- 所有模块编译时自动包含 staging 头文件路径
- 只有 OSAL 模块需要额外包含 `-I$(src)/include` 用于自举编译

### 4. 保持 Makefile 简洁

每个目录的 Makefile 只定义变量，不包含构建规则：

```makefile
# ✅ 推荐：只定义变量
app-y := myapp
myapp-objs := main.o utils.o
ccflags-y += -I$(objtree)/include

# ❌ 不推荐：包含构建规则
app-y := myapp
myapp-objs := main.o utils.o

$(BIN_DIR)/myapp: $(myapp-objs)
	$(CC) -o $@ $^
```

### 5. 正确声明依赖关系

在顶层 Makefile 中声明模块间依赖：

```makefile
# 确保 products 在 core 之后构建
products/: core/

# 确保应用程序在库之后构建
products/ccm/apps/: products/ccm/libs/
```

### 6. 合理组织子目录

使用 `subdir-y` 递归进入子目录：

```makefile
# 始终编译的子目录
subdir-y += src/
subdir-y += libs/

# 条件编译的子目录
subdir-$(CONFIG_TESTS) += tests/
subdir-$(CONFIG_EXAMPLES) += examples/
```

### 7. 导出必要的头文件

库 Makefile 中使用 `header-y` 声明导出的头文件：

```makefile
# 导出公共 API 头文件
header-y := mylib.h mylib_types.h
header-y += api/mylib_api.h

# 不要导出内部头文件
# internal.h、private.h 等不应该在 header-y 中
```

### 8. 配置变更后同步

修改 `.config` 后务必同步配置：

```bash
# 手动同步
make syncconfig

# 或者清理后重新构建（自动同步）
make clean && make
```

### 9. 使用并行编译

充分利用多核 CPU：

```bash
# 使用所有 CPU 核心
make -j$(nproc)

# 或指定核心数
make -j8
```

### 10. 调试时使用详细输出

遇到问题时使用 `V=1` 查看完整命令：

```bash
make V=1
```

## 故障排查

### 配置不生效

**问题**：修改了 `.config` 但编译结果没变化

**解决**：
```bash
make syncconfig
make clean && make
```

### 条件编译不工作

**问题**：`obj-$(CONFIG_XXX)` 没有按预期工作

**检查**：
1. 确认 `CONFIG_XXX` 在 `.config` 中正确设置
2. 运行 `make syncconfig` 同步配置
3. 检查 `include/config/auto.conf` 中的配置值

### 依赖跟踪问题

**问题**：修改头文件后没有重新编译

**解决**：
```bash
make clean && make
```

### 找不到头文件

**问题**：编译时报错找不到头文件

**检查**：
1. 确认库的 Makefile 中声明了 `header-y`
2. 确认应用程序 Makefile 中包含了 `-I$(objtree)/include`
3. 检查 `include/` 目录中是否有对应的头文件
4. 重新构建库以安装头文件：`make clean && make`

### 库名重复前缀

**问题**：`lib-y += libosal.a` 生成了 `liblibosal.a`

**解决**：这个问题已在最新版本中修复，两种写法都正确：
```makefile
lib-y += osal        # → libosal.a
lib-y += libosal.a   # → libosal.a
```

### 并行构建失败

**问题**：`make -j` 时出现依赖错误

**检查**：
1. 确认顶层 Makefile 中声明了模块间依赖（如 `products/: core/`）
2. 确认子目录顺序正确（库在应用程序之前）
3. 尝试单线程构建验证：`make -j1`

### 清理不完全

**问题**：`make clean` 后仍有残留文件

**解决**：
```bash
# 深度清理
make mrproper

# 完全清理
make distclean

# 手动清理（最后手段）
git clean -fdx
```

### Kconfig 工具构建失败

**问题**：`make menuconfig` 失败

**检查**：
1. 确认安装了必要的依赖：`libncurses-dev`、`flex`、`bison`
2. 清理 kconfig 工具：`make mrproper`
3. 重新构建：`make menuconfig`

### 交叉编译失败

**问题**：交叉编译时链接错误

**检查**：
1. 确认交叉编译器路径正确
2. 确认 `CONFIG_CROSS_COMPILE` 设置正确
3. 检查库文件是否为目标架构编译
4. 清理后重新构建：`make mrproper && make menuconfig && make`

## 最近的改进

### 2026-05-21 构建系统优化

1. **Staging 头文件机制**（High Priority）
   - 自动安装库头文件到 `$(objtree)/include/`
   - 简化应用程序 Makefile（减少 35 行硬编码路径）
   - 支持外部构建和 Buildroot/Yocto 集成
   - 实现位置：`scripts/Makefile.build:471-503`

2. **智能库名处理**（Medium Priority）
   - 避免 `lib-y += libosal.a` 变成 `liblibosal.a`
   - 自动检测并保持正确的库名
   - 实现位置：`scripts/Makefile.build:276-278, 329-331`

3. **PRODUCT 变量支持**（Low Priority）
   - `make PRODUCT=ccm` 自动加载配置
   - 简化 CI/CD 流程
   - 实现位置：`Makefile:151-156`

4. **Kconfig 清理机制优化**
   - 参考 Linux 内核实现
   - 支持 hostprogs、targets、subdir- 变量
   - 正确清理 kconfig 工具和生成文件
   - 实现位置：`scripts/Makefile.clean`、`scripts/kconfig/Makefile`

5. **目录结构标准化**
   - 内核模块从 `ko/` 移到 `lib/modules/`
   - 符合 Linux 标准目录布局
   - 实现位置：`Makefile:MODULES_DIR`

6. **清理输出格式修复**
   - 修复双斜杠问题（`core//acl` → `core/acl`）
   - 消除重复目标定义警告
   - 实现位置：`Makefile:core-y, products-y`

## 参考资料

### 内部文档

- [README.md](../README.md) - 项目概述和快速开始
- [CLAUDE.md](../CLAUDE.md) - AI 助手使用指南
- [BUILD_GUIDE.md](BUILD_GUIDE.md) - 构建指南
- [ARCHITECTURE.md](ARCHITECTURE.md) - 架构设计
- [PLATFORM.md](PLATFORM.md) - 平台配置指南
- [OSAL_PLATFORM.md](OSAL_PLATFORM.md) - OSAL 平台适配
- [HAL_PLATFORM.md](HAL_PLATFORM.md) - HAL 平台适配
- [STAGING_DIRECTORY.md](STAGING_DIRECTORY.md) - Staging 目录说明
- [DEFCONFIG_GUIDE.md](DEFCONFIG_GUIDE.md) - 配置文件指南
- [INSTALL.md](INSTALL.md) - 安装指南
- [CODING_STANDARDS.md](CODING_STANDARDS.md) - 编码规范

### 外部资料

- [Linux 内核 Kbuild 文档](https://www.kernel.org/doc/html/latest/kbuild/index.html)
- [Linux 内核 Kconfig 语言](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)
- [Linux 内核 Makefile 约定](https://www.kernel.org/doc/html/latest/kbuild/makefiles.html)

### 源码参考

- 顶层 Makefile：`Makefile`
- 核心构建脚本：`scripts/Makefile.build`
- 清理脚本：`scripts/Makefile.clean`
- Kconfig 配置：`Kconfig`、`core/*/Kconfig`、`products/*/Kconfig`
- Kconfig 工具：`scripts/kconfig/`

---

**最后更新**: 2026-05-21  
**维护者**: wanguo  
**分支**: feature/kconfig-integration
