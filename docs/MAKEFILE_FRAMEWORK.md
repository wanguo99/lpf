# EMS 通用 Makefile 框架使用指南

## 概述

EMS 提供了模块化的通用 Makefile 框架，支持多种构建类型：

- **Makefile.lib** - 构建库（共享库 .so / 静态库 .a）
- **Makefile.app** - 构建应用程序（可执行文件）
- **Makefile.ko** - 构建 Linux 内核模块（.ko）
- **Makefile.dll** - 构建 Windows DLL（.dll）
- **Makefile.rules** - 通用编译规则（被其他框架包含）

## 设计理念

### 1. 模块化设计

每种构建类型有独立的 Makefile：
```
scripts/
├── Makefile.rules    # 通用规则（编译器设置、编译命令）
├── Makefile.lib      # 库构建框架
├── Makefile.app      # 应用程序构建框架
├── Makefile.ko       # 内核模块构建框架
└── Makefile.dll      # Windows DLL 构建框架
```

### 2. 变量驱动

子模块只需定义变量，无需编写编译规则：
```makefile
# 定义变量
LIB_NAME := osal
SRCS := file1.c file2.c

# 包含框架
include $(srctree)/scripts/Makefile.lib
```

### 3. 可扩展性

添加新的构建类型只需创建新的 `Makefile.xxx`：
```bash
# 例如：添加 macOS dylib 支持
cp scripts/Makefile.lib scripts/Makefile.dylib
# 修改 Makefile.dylib 以支持 .dylib 格式
```

## 使用方法

### 1. 构建库 (Makefile.lib)

#### 基本用法

```makefile
# core/mylib/Makefile

# 库配置
LIB_NAME := mylib
LIB_TYPE := shared    # shared/static/both

# 源文件列表
SRCS := \
    mylib.c \
    mylib_utils.c \
    subdir/mylib_extra.c

# 导出的头文件
EXPORT_HEADERS := \
    mylib.h \
    mylib_types.h

# 链接库
LDLIBS := -L$(objtree)/lib -losal

# 包含通用库构建框架
include $(srctree)/scripts/Makefile.lib
```

#### 支持的变量

| 变量 | 必需 | 说明 | 示例 |
|------|------|------|------|
| `LIB_NAME` | ✅ | 库名称（不含 lib 前缀） | `osal` |
| `LIB_TYPE` | ❌ | 库类型 | `shared`/`static`/`both` (默认 `shared`) |
| `SRCS` | ✅ | 源文件列表 | `file1.c file2.c` |
| `EXPORT_HEADERS` | ❌ | 导出的头文件 | `mylib.h mylib_types.h` |
| `SRC_DIR` | ❌ | 源文件目录 | 默认 `$(srctree)/core/$(LIB_NAME)/src` |
| `INC_DIR` | ❌ | 头文件目录 | 默认 `$(srctree)/core/$(LIB_NAME)/include` |
| `CFLAGS_LIB` | ❌ | 库特定编译标志 | `-DMYLIB_DEBUG` |
| `LDLIBS` | ❌ | 链接库 | `-lpthread -lrt` |

#### 条件编译

```makefile
# 根据 Kconfig 配置条件编译
SRCS-$(CONFIG_FEATURE_A) += feature_a.c
SRCS-$(CONFIG_FEATURE_B) += feature_b.c
```

#### 构建命令

```bash
# 构建库
make -C core/mylib

# 导出头文件到顶层 include/
make -C core/mylib headers_install

# 清理
make -C core/mylib clean
```

### 2. 构建应用程序 (Makefile.app)

#### 基本用法

```makefile
# products/myapp/Makefile

# 应用配置
APP_NAME := myapp

# 源文件列表
SRCS := \
    main.c \
    app_logic.c \
    app_utils.c

# 链接库
LDLIBS := -L$(objtree)/lib -losal -lhal -lpdl

# 包含通用应用构建框架
include $(srctree)/scripts/Makefile.app
```

#### 支持的变量

| 变量 | 必需 | 说明 | 示例 |
|------|------|------|------|
| `APP_NAME` | ✅ | 应用程序名称 | `sample_app` |
| `SRCS` | ✅ | 源文件列表 | `main.c logic.c` |
| `SRC_DIR` | ❌ | 源文件目录 | 默认 `$(srctree)/products/$(APP_NAME)/src` |
| `INC_DIR` | ❌ | 头文件目录 | 默认 `$(srctree)/products/$(APP_NAME)/include` |
| `CFLAGS_APP` | ❌ | 应用特定编译标志 | `-DAPP_VERSION=1.0` |
| `LDFLAGS` | ❌ | 链接标志 | `-Wl,-rpath,/opt/ems/lib` |
| `LDLIBS` | ❌ | 链接库 | `-losal -lhal` |

#### 构建命令

```bash
# 构建应用
make -C products/myapp

# 安装
make -C products/myapp install INSTALL_PREFIX=/opt/ems

# 清理
make -C products/myapp clean
```

### 3. 构建内核模块 (Makefile.ko)

#### 基本用法

```makefile
# drivers/mydriver/Makefile

# 内核模块配置
KO_NAME := mydriver

# 源文件列表
SRCS := \
    mydriver_main.c \
    mydriver_device.c \
    mydriver_ops.c

# 内核源码目录（可选）
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# 包含通用内核模块构建框架
include $(srctree)/scripts/Makefile.ko
```

#### 支持的变量

| 变量 | 必需 | 说明 | 示例 |
|------|------|------|------|
| `KO_NAME` | ✅ | 内核模块名称 | `ems_driver` |
| `SRCS` | ✅ | 源文件列表 | `main.c device.c` |
| `KERNEL_DIR` | ❌ | 内核源码目录 | 默认 `/lib/modules/$(uname -r)/build` |
| `SRC_DIR` | ❌ | 源文件目录 | 默认 `$(srctree)/drivers/$(KO_NAME)/src` |
| `CFLAGS_KO` | ❌ | 模块特定编译标志 | `-DDRIVER_DEBUG` |

#### 构建命令

```bash
# 构建内核模块
make -C drivers/mydriver

# 安装到系统
sudo make -C drivers/mydriver modules_install

# 加载模块
sudo make -C drivers/mydriver load

# 卸载模块
sudo make -C drivers/mydriver unload

# 重新加载
sudo make -C drivers/mydriver reload

# 清理
make -C drivers/mydriver clean
```

### 4. 构建 Windows DLL (Makefile.dll)

#### 基本用法

```makefile
# core/mylib/Makefile.windows

# DLL 配置
DLL_NAME := mylib

# 源文件列表
SRCS := \
    mylib.c \
    mylib_utils.c \
    mylib_win32.c

# 导出的头文件
EXPORT_HEADERS := \
    mylib.h \
    mylib_types.h

# 链接库
LDLIBS := -lws2_32 -lwinmm

# 包含通用 DLL 构建框架
include $(srctree)/scripts/Makefile.dll
```

#### 支持的变量

| 变量 | 必需 | 说明 | 示例 |
|------|------|------|------|
| `DLL_NAME` | ✅ | DLL 名称（不含后缀） | `osal` |
| `SRCS` | ✅ | 源文件列表 | `file1.c file2.c` |
| `EXPORT_HEADERS` | ❌ | 导出的头文件 | `mylib.h` |
| `DEF_FILE` | ❌ | .def 文件（导出符号） | `mylib.def` |
| `CFLAGS_DLL` | ❌ | DLL 特定编译标志 | `-DDLL_EXPORT` |
| `LDLIBS` | ❌ | 链接库 | `-lws2_32` |

#### 构建命令

```bash
# 设置交叉编译工具链
export CROSS_COMPILE=x86_64-w64-mingw32-

# 构建 DLL
make -C core/mylib -f Makefile.windows

# 查看构建信息
make -C core/mylib -f Makefile.windows info

# 安装
make -C core/mylib -f Makefile.windows install INSTALL_PREFIX=/opt/ems

# 清理
make -C core/mylib -f Makefile.windows clean
```

## 完整示例

### 示例 1：构建共享库

```makefile
# core/mylib/Makefile

LIB_NAME := mylib
LIB_TYPE := shared

SRCS := \
    core/mylib_core.c \
    utils/mylib_utils.c \
    platform/linux/mylib_linux.c

EXPORT_HEADERS := \
    mylib.h \
    mylib_types.h \
    utils/mylib_utils.h

CFLAGS_LIB := -DMYLIB_VERSION=\"1.0.0\"
LDLIBS := -L$(objtree)/lib -losal -lpthread

include $(srctree)/scripts/Makefile.lib
```

### 示例 2：构建静态库和共享库

```makefile
# core/mylib/Makefile

LIB_NAME := mylib
LIB_TYPE := both    # 同时构建 .so 和 .a

SRCS := mylib.c mylib_utils.c

EXPORT_HEADERS := mylib.h

include $(srctree)/scripts/Makefile.lib
```

输出：
- `lib/libmylib.so` - 共享库
- `lib/libmylib.a` - 静态库

### 示例 3：条件编译

```makefile
# core/hal/Makefile

LIB_NAME := hal
LIB_TYPE := shared

# 基础源文件
SRCS := hal_common.c

# 条件编译：根据 Kconfig 配置
SRCS-$(CONFIG_HAL_CAN) += hal_can.c
SRCS-$(CONFIG_HAL_UART) += hal_serial.c
SRCS-$(CONFIG_HAL_I2C) += hal_i2c.c

EXPORT_HEADERS := \
    hal.h \
    hal_can.h \
    hal_serial.h \
    hal_i2c.h

LDLIBS := -L$(objtree)/lib -losal

include $(srctree)/scripts/Makefile.lib
```

### 示例 4：构建应用程序

```makefile
# products/sample_app/Makefile

APP_NAME := sample_app

SRCS := \
    main.c \
    app_init.c \
    app_logic.c \
    app_cleanup.c

CFLAGS_APP := -DAPP_NAME=\"Sample Application\"
LDLIBS := -L$(objtree)/lib -losal -lhal -lpdl -lacl

include $(srctree)/scripts/Makefile.app
```

### 示例 5：构建内核模块

```makefile
# drivers/ems_can/Makefile

KO_NAME := ems_can

SRCS := \
    ems_can_main.c \
    ems_can_device.c \
    ems_can_ops.c

CFLAGS_KO := -DDRIVER_VERSION=\"1.0.0\"

include $(srctree)/scripts/Makefile.ko
```

## 添加新的构建类型

### 步骤 1：创建新的 Makefile

```bash
# 例如：添加 macOS dylib 支持
cp scripts/Makefile.lib scripts/Makefile.dylib
```

### 步骤 2：修改构建规则

```makefile
# scripts/Makefile.dylib

# 修改目标文件名
TARGET := $(LIB_DIR)/lib$(LIB_NAME).dylib

# 修改链接命令
quiet_cmd_ld_dylib = LD [DYLIB] $@
      cmd_ld_dylib = $(CC) -dynamiclib -o $@ $^ $(LDFLAGS) $(LDLIBS)

# 修改构建目标
$(TARGET): $(OBJS) | $(LIB_DIR)
	$(call cmd,ld_dylib)
```

### 步骤 3：在模块中使用

```makefile
# core/mylib/Makefile.macos

LIB_NAME := mylib
SRCS := mylib.c

include $(srctree)/scripts/Makefile.dylib
```

## 最佳实践

### 1. 目录结构

```
core/mylib/
├── Makefile              # Linux 共享库
├── Makefile.static       # Linux 静态库
├── Makefile.windows      # Windows DLL
├── include/              # 头文件
│   ├── mylib.h
│   └── mylib_types.h
└── src/                  # 源文件
    ├── mylib.c
    └── platform/
        ├── linux/
        │   └── mylib_linux.c
        └── windows/
            └── mylib_win32.c
```

### 2. 条件编译

使用 Kconfig 配置进行条件编译：

```makefile
# 推荐：使用 SRCS-$(CONFIG_XXX)
SRCS-$(CONFIG_FEATURE_A) += feature_a.c
SRCS-$(CONFIG_FEATURE_B) += feature_b.c

# 不推荐：使用 ifeq
ifeq ($(CONFIG_FEATURE_A),y)
  SRCS += feature_a.c
endif
```

### 3. 头文件导出

只导出公共 API 头文件：

```makefile
# 导出公共头文件
EXPORT_HEADERS := \
    mylib.h \
    mylib_types.h

# 不导出内部头文件
# mylib_internal.h - 不在 EXPORT_HEADERS 中
```

### 4. 依赖管理

明确声明库依赖：

```makefile
# 应用程序依赖多个库
LDLIBS := -L$(objtree)/lib -losal -lhal -lpdl

# 库依赖其他库
LDLIBS := -L$(objtree)/lib -losal -lpthread
```

## 常见问题

### Q: 如何同时构建共享库和静态库？

A: 设置 `LIB_TYPE := both`

```makefile
LIB_NAME := mylib
LIB_TYPE := both
SRCS := mylib.c
include $(srctree)/scripts/Makefile.lib
```

### Q: 如何添加自定义编译标志？

A: 使用 `CFLAGS_LIB`、`CFLAGS_APP` 等变量

```makefile
CFLAGS_LIB := -DDEBUG -DVERSION=\"1.0\"
```

### Q: 如何支持 C++ 源文件？

A: 框架自动支持 `.cpp` 文件，使用 `CXX` 编译器

```makefile
SRCS := mylib.cpp mylib_utils.cpp
```

### Q: 如何查看详细的编译命令？

A: 使用 `V=1` 参数

```bash
make V=1
```

### Q: 如何交叉编译？

A: 设置 `CROSS_COMPILE` 环境变量

```bash
export CROSS_COMPILE=arm-linux-gnueabihf-
make
```

## 总结

EMS 通用 Makefile 框架提供了：

1. ✅ **模块化设计** - 每种构建类型独立的 Makefile
2. ✅ **变量驱动** - 子模块只需定义变量
3. ✅ **可扩展** - 易于添加新的构建类型
4. ✅ **条件编译** - 原生支持 Kconfig
5. ✅ **跨平台** - 支持 Linux、Windows、内核模块
6. ✅ **自动化** - 编译、链接、依赖跟踪全自动

这套框架大大简化了模块开发，提高了构建系统的可维护性。
