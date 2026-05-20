# EMS 构建系统文档

## 概述

EMS 项目采用 Linux 内核风格的 Kbuild 构建系统，支持：
- Kconfig 配置管理
- 条件编译（`obj-$(CONFIG_XXX)` 语法）
- 递归子目录构建
- 依赖跟踪
- 静默编译输出

## 条件编译机制

### obj-$(CONFIG_XXX) 语法

参考 Linux 内核，EMS 支持使用 `obj-$(CONFIG_XXX)` 语法进行条件编译：

```makefile
# 示例：OSAL 网络模块
obj-$(CONFIG_OSAL_NETWORK) += osal_socket.o
obj-$(CONFIG_OSAL_NETWORK) += osal_termios.o
```

当 `CONFIG_OSAL_NETWORK=y` 时，这些文件会被编译；当配置为 `n` 时，这些文件会被跳过。

### 自动展开机制

构建系统会自动展开 `obj-$(CONFIG_XXX)` 变量：

1. 在 `scripts/build/Makefile.rules` 中实现自动展开逻辑
2. 在 `scripts/build/Makefile.build` 和 `Makefile.subdir` 中收集所有 obj-y
3. 支持 `obj-$(CONFIG_XXX)`、`lib-$(CONFIG_XXX)`、`subdir-$(CONFIG_XXX)`

### 使用示例

#### OSAL 子功能条件编译

```makefile
# core/osal/src/posix/sys/Makefile
obj-y += osal_clock.o                    # 始终编译
obj-$(CONFIG_OSAL_FILE) += osal_file.o   # 条件编译
obj-$(CONFIG_OSAL_SIGNAL) += osal_signal.o
obj-$(CONFIG_OSAL_THREAD) += osal_thread.o
obj-$(CONFIG_OSAL_TIME) += osal_time.o
```

#### HAL 驱动条件编译

```makefile
# core/hal/src/linux/Makefile
obj-$(CONFIG_HAL_CAN) += hal_can_linux.o
obj-$(CONFIG_HAL_UART) += hal_serial_linux.o
obj-$(CONFIG_HAL_I2C) += hal_i2c_linux.o
obj-$(CONFIG_HAL_SPI) += hal_spi_linux.o
obj-$(CONFIG_HAL_GPIO) += hal_gpio_linux.o
obj-$(CONFIG_HAL_WATCHDOG) += hal_watchdog_linux.o
```

## Makefile 编写规范

### 基本结构

```makefile
# 模块注释

# 条件编译的源文件
obj-$(CONFIG_XXX) += file1.o
obj-$(CONFIG_YYY) += file2.o

# 始终编译的源文件
obj-y += file3.o

# 子目录递归
subdir-y += subdir1/
subdir-$(CONFIG_ZZZ) += subdir2/
```

### 库构建

```makefile
# 静态库
obj-s := libxxx.a
libxxx.a-objs := $(obj-y)

# 动态库
obj-d := libxxx.so
libxxx.so-objs := $(obj-y)

# 包含构建框架
include $(scriptdir)/build/Makefile.build
```

### 应用程序构建

```makefile
# 可执行文件
obj-e := app_name
app_name-objs := main.o module.o

# 链接库
LDLIBS := -losal -lhal -lpthread

# 包含构建框架
include $(scriptdir)/build/Makefile.build
```

## 配置管理

### 配置文件

- `.config` - 用户配置文件
- `include/config/auto.conf` - 自动生成的 Makefile 配置
- `include/generated/autoconf.h` - 自动生成的 C 头文件

### 配置命令

```bash
# 图形化配置
make menuconfig

# 同步配置（.config -> auto.conf）
make syncconfig

# 使用默认配置
make defconfig
```

### 配置变更流程

1. 修改 `.config` 或运行 `make menuconfig`
2. 运行 `make syncconfig` 同步配置
3. 重新编译：`make clean && make`

## 构建目标

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

### 清理

```bash
make clean         # 清理所有构建产物
make mrproper      # 清理包括配置文件
```

## 依赖跟踪

构建系统自动跟踪头文件依赖：

- 使用 `-MMD -MP` 编译标志生成 `.d` 文件
- 自动包含 `.d` 文件以跟踪依赖关系
- 头文件修改后自动重新编译相关源文件

## 静默编译

默认使用静默编译输出，只显示简洁的编译信息：

```
  CC      core/osal/src/posix/lib/osal_errno.o
  AR      lib/libosal.a
  LD      lib/libosal.so
```

使用 `V=1` 查看完整编译命令：

```bash
make V=1
```

## 交叉编译

通过 Kconfig 配置交叉编译器：

```bash
make menuconfig
# 设置 "Cross-compiler tool prefix"
# 例如：arm-linux-gnueabihf-
```

或直接修改 `.config`：

```
CONFIG_CROSS_COMPILE="arm-linux-gnueabihf-"
```

## 最佳实践

### 1. 使用条件编译

优先使用 `obj-$(CONFIG_XXX)` 而不是 `ifeq` 判断：

```makefile
# 推荐
obj-$(CONFIG_FEATURE) += feature.o

# 不推荐
ifeq ($(CONFIG_FEATURE),y)
  obj-y += feature.o
endif
```

### 2. 保持 Makefile 简洁

每个目录的 Makefile 只定义变量，不包含构建规则：

```makefile
# 只定义 obj-y、obj-s、obj-d、obj-e
obj-y += file1.o file2.o

# 包含统一的构建框架
include $(scriptdir)/build/Makefile.build
```

### 3. 合理组织子目录

使用 `subdir-y` 递归进入子目录：

```makefile
subdir-y += src/
subdir-$(CONFIG_TESTS) += tests/
```

### 4. 配置变更后同步

修改 `.config` 后务必运行 `make syncconfig`，否则配置不会生效。

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

## 参考资料

- Linux 内核 Kbuild 文档：`Documentation/kbuild/`
- EMS Kconfig 文件：`Kconfig`、`core/*/Kconfig`、`products/*/Kconfig`
- 构建脚本：`scripts/build/Makefile.*`
