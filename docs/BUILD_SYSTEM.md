# EMS 构建系统设计文档

## 概述

EMS 采用 **Linux 内核风格的 Kconfig + Makefile** 构建系统，提供灵活的配置管理和高效的增量编译。

## 设计理念

### 1. Linux 内核风格

参考 Linux 内核构建系统的设计：
- **Kconfig**: 菜单驱动的配置系统
- **静默编译**: 清晰的编译输出（CC、LD、AR 前缀）
- **依赖跟踪**: 自动生成 .d 文件跟踪头文件依赖
- **Out-of-tree 构建**: 支持源码树外编译
- **模块化**: 每个模块独立配置和编译

### 2. Buildroot Package Infrastructure

借鉴 Buildroot 的 package infrastructure 设计：
- **通用构建框架**: `scripts/Makefile.build` 提供统一的构建规则
- **简化子模块**: 子模块 Makefile 只需定义变量（MODULE_NAME、SRCS、EXPORT_HEADERS）
- **自动化处理**: 编译、链接、依赖跟踪、头文件导出全部自动化

## 目录结构

```
EMS/
├── Makefile                    # 顶层 Makefile（配置、构建、安装）
├── Kconfig                     # 顶层配置菜单
├── defconfig                   # 默认配置
├── configs/                    # 预设配置文件
│   ├── ccm_h200_am625_release_defconfig
│   └── ccm_h200_am625_debug_defconfig
├── include/                    # 顶层头文件目录（导出的 API）
│   ├── osal/                   # OSAL 公共头文件
│   ├── hal/                    # HAL 公共头文件
│   ├── pdl/                    # PDL 公共头文件
│   ├── pcl/                    # PCL 公共头文件
│   ├── acl/                    # ACL 公共头文件
│   ├── generated/              # 生成的配置头文件
│   │   └── autoconf.h          # Kconfig 生成的宏定义
│   └── ems_config.h            # EMS 全局配置
├── core/                       # 核心模块
│   ├── Makefile                # 核心模块协调器
│   ├── osal/                   # OSAL 模块
│   │   ├── Kconfig             # OSAL 配置选项
│   │   ├── Makefile            # OSAL 构建脚本
│   │   ├── include/            # OSAL 内部头文件
│   │   └── src/posix/          # POSIX 实现
│   ├── hal/                    # HAL 模块
│   ├── pcl/                    # PCL 模块
│   ├── pdl/                    # PDL 模块
│   └── acl/                    # ACL 模块
├── products/                   # 产品模块
│   ├── Makefile
│   ├── ccm/                    # CCM 产品
│   └── samples/                # 示例应用
├── scripts/                    # 构建脚本
│   ├── Makefile.lib            # 通用编译规则库
│   ├── Makefile.build          # 通用构建框架
│   ├── install.mk              # 安装脚本
│   ├── headers_install.sh      # 头文件安装脚本
│   └── kconfig/                # Kconfig 工具
├── lib/                        # 编译输出：库文件
├── bin/                        # 编译输出：可执行文件
└── .config                     # 当前配置（生成，不提交）
```

## 核心组件

### 1. Kconfig 配置系统

#### 配置文件层次

```
Kconfig (顶层)
├── core/osal/Kconfig       # OSAL 配置
├── core/hal/Kconfig        # HAL 配置
├── core/pcl/Kconfig        # PCL 配置
├── core/pdl/Kconfig        # PDL 配置
├── core/acl/Kconfig        # ACL 配置
├── products/ccm/Kconfig    # CCM 产品配置
└── products/samples/Kconfig # 示例配置
```

#### 模块依赖关系

```kconfig
config OSAL
    bool "OSAL"
    default y

config HAL
    bool "HAL"
    depends on OSAL
    default y

config PCL
    bool "PCL"
    depends on OSAL && HAL
    default y

config PDL
    bool "PDL"
    depends on OSAL && HAL && PCL
    default y

config ACL
    bool "ACL"
    depends on OSAL && PDL
    default y
```

#### 配置命令

```bash
make menuconfig          # 交互式配置（推荐）
make defconfig           # 加载默认配置
make ccm_h200_am625_release_defconfig  # 加载预设配置
make savedefconfig       # 保存最小化配置
make oldconfig           # 更新配置（新选项）
make syncconfig          # 同步配置并生成头文件
```

### 2. 通用构建框架 (scripts/Makefile.build)

#### 设计目标

- 子模块 Makefile 只需定义变量，无需编写编译规则
- 自动处理编译、链接、依赖跟踪、头文件导出
- 支持共享库、静态库、可执行文件三种类型
- 支持条件编译（基于 Kconfig）

#### 使用方法

子模块 Makefile 示例：

```makefile
# 模块配置
MODULE_NAME := osal
MODULE_TYPE := shared_lib    # shared_lib / static_lib / executable

# 源文件目录（默认：$(srctree)/core/$(MODULE_NAME)/src）
SRC_DIR := $(srctree)/core/$(MODULE_NAME)/src/posix

# 源文件列表（相对于 SRC_DIR）
SRCS := \
    lib/osal_errno.c \
    ipc/osal_mutex.c \
    sys/osal_file.c

# 导出的头文件（会被复制到顶层 include/$(MODULE_NAME)/）
EXPORT_HEADERS := \
    osal.h \
    osal_types.h \
    ipc/osal_mutex.h

# 模块特定的编译标志
CFLAGS_MODULE := -DOSAL_DEBUG

# 链接库
LDLIBS := -lpthread -lrt

# 包含通用构建框架
include $(srctree)/scripts/Makefile.build
```

#### 条件编译支持

```makefile
# 根据 Kconfig 配置条件编译
SRCS-$(CONFIG_HAL_CAN) += hal_can.c
SRCS-$(CONFIG_HAL_UART) += hal_serial.c
SRCS-$(CONFIG_HAL_I2C) += hal_i2c.c

# 过滤并生成最终源文件列表
SRCS := $(call filter-srcs,$(SRCS-y))
```

### 3. 头文件管理

#### 三层头文件结构

1. **模块内部头文件**: `core/*/include/`
   - 模块开发时使用
   - 包含内部实现细节

2. **顶层导出头文件**: `include/*/`
   - 跨模块引用时使用
   - 只包含公共 API
   - 通过 `make headers_install` 从模块内部导出

3. **系统安装头文件**: `/usr/local/include/`
   - 外部应用使用
   - 通过 `make headers_install INSTALL_PREFIX=/usr/local` 安装

#### 头文件导出流程

```
core/osal/include/osal.h
    ↓ (make headers_install in core/osal)
include/osal/osal.h
    ↓ (make headers_install in top-level)
/usr/local/include/osal/osal.h
```

#### 头文件引用规则

```c
// 模块内部引用（开发时）
#include "osal_internal.h"

// 跨模块引用（使用顶层 include/）
#include <osal/osal.h>
#include <hal/hal_can.h>

// 使用生成的配置
#include <generated/autoconf.h>
#ifdef CONFIG_HAL_CAN
    // CAN 相关代码
#endif
```

### 4. 依赖跟踪

#### 自动依赖生成

编译时自动生成 `.d` 文件：

```makefile
# Makefile.lib 中的设置
CFLAGS += -MMD -MP

# 包含依赖文件
-include $(OBJS:.o=.d)
```

#### 依赖文件示例

```makefile
# core/osal/lib/osal_errno.d
core/osal/lib/osal_errno.o: \
  core/osal/src/posix/lib/osal_errno.c \
  core/osal/include/lib/osal_errno.h \
  core/osal/include/osal_types.h \
  include/generated/autoconf.h
```

当头文件修改时，相关的 `.o` 文件会自动重新编译。

### 5. 静默编译

#### 输出格式

```
  CC      core/osal/lib/osal_errno.o
  CC      core/osal/ipc/osal_mutex.o
  LD      lib/libosal.so
  CC      core/hal/hal_can.o
  LD      lib/libhal.so
```

#### 详细输出

```bash
make V=1    # 显示完整的编译命令
```

## 构建流程

### 1. 配置阶段

```bash
# 选择配置
make menuconfig
# 或
make ccm_h200_am625_release_defconfig

# 生成文件：
# - .config                          # 配置文件
# - include/generated/autoconf.h     # C 宏定义
# - include/config/auto.conf         # Makefile 变量
```

### 2. 编译阶段

```bash
make -j$(nproc)

# 编译顺序（按依赖关系）：
# 1. OSAL
# 2. HAL (依赖 OSAL)
# 3. PCL (依赖 OSAL, HAL)
# 4. PDL (依赖 OSAL, HAL, PCL)
# 5. ACL (依赖 OSAL, PDL)
# 6. Products (依赖所有 core 模块)
```

### 3. 安装阶段

```bash
# 安装库和二进制文件
make install INSTALL_PREFIX=/opt/ems

# 仅安装头文件
make headers_install INSTALL_PREFIX=/opt/ems

# 完整安装
make install-all INSTALL_PREFIX=/opt/ems

# 打包安装（DESTDIR）
make install-all DESTDIR=/tmp/staging INSTALL_PREFIX=/usr
```

## Out-of-tree 构建

### 使用方法

```bash
# 在独立目录构建
make O=/tmp/ems-build defconfig
make O=/tmp/ems-build -j$(nproc)

# 目录结构：
/tmp/ems-build/
├── .config
├── include/generated/autoconf.h
├── core/                    # 目标文件
├── products/                # 目标文件
├── lib/                     # 库文件
└── bin/                     # 可执行文件

# 源码树保持干净
```

### 优势

- 源码树保持干净（无编译产物）
- 支持多个构建配置（debug/release）
- 便于交叉编译

## 模块开发指南

### 添加新模块

1. **创建模块目录**

```bash
mkdir -p core/newmodule/{include,src}
```

2. **创建 Kconfig**

```kconfig
# core/newmodule/Kconfig
config NEWMODULE
    bool "New Module"
    depends on OSAL
    default y
    help
      Description of the new module.
```

3. **创建 Makefile**

```makefile
# core/newmodule/Makefile
MODULE_NAME := newmodule
MODULE_TYPE := shared_lib

SRCS := \
    newmodule.c \
    newmodule_utils.c

EXPORT_HEADERS := \
    newmodule.h

LDLIBS := -L$(objtree)/lib -losal

include $(srctree)/scripts/Makefile.build
```

4. **更新顶层 Kconfig**

```kconfig
# Kconfig
source "core/newmodule/Kconfig"
```

5. **更新 core/Makefile**

```makefile
# core/Makefile
MODULES := osal hal pcl pdl acl newmodule

# 添加依赖关系
newmodule: osal
```

### 添加条件编译特性

1. **在 Kconfig 中添加选项**

```kconfig
config HAL_NEW_DRIVER
    bool "New driver support"
    depends on HAL
    default n
```

2. **在 Makefile 中条件编译**

```makefile
SRCS-$(CONFIG_HAL_NEW_DRIVER) += hal_new_driver.c
SRCS := $(call filter-srcs,$(SRCS-y))
```

3. **在代码中使用**

```c
#include <generated/autoconf.h>

#ifdef CONFIG_HAL_NEW_DRIVER
    // 新驱动代码
#endif
```

## 最佳实践

### 1. 模块设计

- **单一职责**: 每个模块只负责一个功能域
- **最小依赖**: 只依赖必需的模块
- **清晰接口**: 导出的头文件只包含公共 API

### 2. 头文件管理

- **内部头文件**: 放在 `include/` 下，不导出
- **公共头文件**: 通过 `EXPORT_HEADERS` 导出到顶层
- **避免循环依赖**: 使用前向声明

### 3. 编译优化

- **并行编译**: 使用 `make -j$(nproc)`
- **增量编译**: 依赖跟踪自动处理
- **Out-of-tree**: 多配置构建时使用

### 4. 配置管理

- **预设配置**: 为常用配置创建 defconfig
- **最小化配置**: 使用 `savedefconfig` 保存
- **版本控制**: 只提交 defconfig，不提交 .config

## 常见问题

### Q: 如何添加新的源文件？

A: 在模块的 Makefile 中的 `SRCS` 变量添加即可，无需修改编译规则。

### Q: 如何导出新的头文件？

A: 在模块的 Makefile 中的 `EXPORT_HEADERS` 变量添加，然后运行 `make headers_install`。

### Q: 如何添加模块间依赖？

A: 在 Kconfig 中使用 `depends on`，在 core/Makefile 中添加依赖关系。

### Q: 如何调试编译问题？

A: 使用 `make V=1` 查看完整的编译命令。

### Q: 如何清理构建？

A:
- `make clean`: 清理编译产物
- `make distclean`: 清理编译产物和配置
- `make mrproper`: 完全清理（包括 kconfig 工具）

## 参考资料

- Linux 内核构建系统: https://www.kernel.org/doc/html/latest/kbuild/
- Kconfig 语言: https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html
- Buildroot 手册: https://buildroot.org/downloads/manual/manual.html
