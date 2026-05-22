# EMS 构建指南

本文档提供 EMS 项目的完整构建说明，包括配置、编译、交叉编译和清理。

## 快速开始

### 基本构建流程

```bash
# 1. 配置
make menuconfig
# 或使用预定义配置
make ccm_h200_am625_debug_defconfig

# 2. 编译
make -j$(nproc)

# 3. 查看输出
ls -lh bin/
ls -lh lib/
```

### 使用 PRODUCT 变量（推荐）

```bash
# 一步完成配置和编译
make PRODUCT=ccm_h200_am625_debug
make -j$(nproc)
```

## 配置系统

### 图形化配置

```bash
# 启动 menuconfig 界面
make menuconfig
```

**导航方式**：
- 方向键：上下移动
- Enter：进入子菜单或选择
- Space：切换选项（Y/N/M）
- `/`：搜索配置项
- `?`：显示帮助信息
- Esc Esc：返回上级或退出

**主要配置项**：
- Build Options：编译器、优化级别、交叉编译
- Core Modules：OSAL、HAL、PCL、PDL、ACL
- Products：CCM 产品及其应用程序
- Debugging Options：调试符号、日志级别

### 预定义配置

```bash
# 查看可用配置
ls configs/

# 使用预定义配置
make ccm_h200_am625_debug_defconfig
make ccm_h200_am625_release_defconfig
make ccm_h200_am625_test_defconfig

# 使用 PRODUCT 变量（自动加载对应的 defconfig）
make PRODUCT=ccm_h200_am625_debug
```

### 保存配置

```bash
# 保存最小化配置到 configs/
make savedefconfig

# 配置文件会保存为 defconfig
# 手动重命名为产品配置
mv defconfig configs/my_product_defconfig
```

### 配置文件说明

| 文件 | 用途 |
|------|------|
| `.config` | 完整的用户配置（手动编辑或 menuconfig 生成） |
| `configs/*_defconfig` | 最小化的预定义配置 |
| `include/config/auto.conf` | 自动生成的 Makefile 配置 |
| `include/generated/autoconf.h` | 自动生成的 C 头文件 |

## 编译

### 完整编译

```bash
# 编译所有目标
make

# 并行编译（推荐）
make -j$(nproc)

# 详细输出（查看完整命令）
make V=1

# 静默输出
make V=0
```

### 部分编译

```bash
# 只编译核心模块
make core

# 只编译特定模块
make core/osal
make core/hal
make core/pcl
make core/pdl
make core/acl

# 只编译产品模块
make products

# 只编译特定产品
make products/ccm
```

### 编译输出

```
  CC      core/osal/osal_thread.o
  CC      core/osal/osal_mutex.o
  AR      lib/libosal.a
  CC      core/hal/hal_can_linux.o
  AR      lib/libhal.a
  CC      products/ccm/apps/ccm_collector/main.o
  LD      bin/ccm_collector
```

**输出格式说明**：
- `CC`：编译 C 源文件
- `AR`：创建静态库
- `LD`：链接可执行文件或动态库
- `INSTALL`：安装文件到输出目录

## 输出目录结构

```
EMS/
├── bin/                      # 可执行文件
│   ├── ccm_collector
│   ├── ccm_logger
│   ├── ccm_health
│   ├── ccm_supervisor
│   └── ccm_comm
├── lib/                      # 库文件
│   ├── libosal.a
│   ├── libhal.a
│   ├── libpcl.a
│   ├── libpdl.a
│   ├── libacl.a
│   ├── libccm.so
│   └── modules/             # 内核模块
│       └── *.ko
└── include/                 # Staging 头文件（自动生成）
    ├── osal.h
    ├── hal.h
    ├── pcl.h
    ├── pdl.h
    ├── acl.h
    ├── ccm.h
    ├── ipc/
    ├── lib/
    ├── net/
    ├── sys/
    └── util/
```

## 清理

### 清理级别

```bash
# 1. 清理编译产物（保留配置）
make clean
# 删除：bin/、lib/、include/、*.o、*.a、*.so、.*.cmd

# 2. 深度清理（删除配置和工具）
make mrproper
# 额外删除：.config、include/config/、scripts/kconfig/conf、mconf

# 3. 完全清理（包括临时文件）
make distclean
# 额外删除：*~、*.swp、*.bak
```

### 清理后重新构建

```bash
# 完整的清理和重建流程
make mrproper
make ccm_h200_am625_debug_defconfig
make -j$(nproc)
```

## 交叉编译

### 通过 Kconfig 配置

```bash
# 1. 启动配置界面
make menuconfig

# 2. 导航到 "Build Options"
# 3. 设置 "Cross-compiler tool prefix"
#    例如：aarch64-linux-gnu-

# 4. 保存并退出

# 5. 编译
make -j$(nproc)
```

### 命令行指定

```bash
# 临时指定交叉编译器前缀
make CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)

# 或直接指定编译器
make CC=aarch64-linux-gnu-gcc LD=aarch64-linux-gnu-ld -j$(nproc)
```

### 常用交叉编译器

| 目标平台 | 工具链前缀 | 安装命令 (Ubuntu/Debian) |
|---------|-----------|------------------------|
| ARM 32-bit | `arm-linux-gnueabihf-` | `sudo apt install gcc-arm-linux-gnueabihf` |
| ARM 64-bit | `aarch64-linux-gnu-` | `sudo apt install gcc-aarch64-linux-gnu` |
| TI AM62x | `aarch64-none-linux-gnu-` | 使用 TI SDK 工具链 |
| RISC-V 64 | `riscv64-linux-gnu-` | `sudo apt install gcc-riscv64-linux-gnu` |
| MIPS | `mips-linux-gnu-` | `sudo apt install gcc-mips-linux-gnu` |

### TI AM62x 交叉编译示例

```bash
# 1. 设置 TI SDK 环境
export PATH=/path/to/ti-sdk/bin:$PATH

# 2. 配置
make menuconfig
# 设置 Cross-compiler tool prefix: aarch64-none-linux-gnu-

# 3. 编译
make -j$(nproc)

# 4. 部署到目标板
scp bin/* root@192.168.1.100:/usr/local/bin/
scp lib/*.so root@192.168.1.100:/usr/local/lib/
```

## 外部构建

支持在独立目录中构建，保持源码目录干净：

```bash
# 方法 1：使用 O= 参数
mkdir ../build
make O=../build menuconfig
make O=../build -j$(nproc)

# 方法 2：在构建目录中执行
mkdir ../build
cd ../build
make -C /path/to/EMS menuconfig
make -C /path/to/EMS -j$(nproc)
```

**外部构建的优势**：
- 源码目录保持干净
- 可以同时维护多个构建配置
- 便于清理（直接删除构建目录）

## 调试构建

### 查看详细输出

```bash
# 显示完整的编译命令
make V=1

# 显示 Make 调试信息
make --debug=v

# 显示但不执行命令（dry-run）
make -n
```

### 查看变量值

```bash
# 显示所有 Make 变量
make -p | less

# 查找特定变量
make -p | grep "^app-y"
make -p | grep "^lib-y"
```

### 检查依赖文件

```bash
# 查看目标文件的依赖
cat core/osal/.osal_thread.o.d

# 查看编译命令
cat core/osal/.osal_thread.o.cmd
```

## 构建选项

### 编译器选项

在 menuconfig 中配置或直接在命令行指定：

```bash
# 优化级别
make CFLAGS_OPTIMIZE="-O2"
make CFLAGS_OPTIMIZE="-O3"
make CFLAGS_OPTIMIZE="-Os"  # 优化大小

# 调试符号
make CFLAGS_DEBUG="-g"
make CFLAGS_DEBUG="-g3"     # 包含宏定义

# 警告选项
make CFLAGS_WARN="-Wall -Werror"
```

### 构建类型

```bash
# Debug 构建（包含调试符号，禁用优化）
make menuconfig
# 选择 "Build Options" -> "Build Type" -> "Debug"

# Release 构建（优化，无调试符号）
make menuconfig
# 选择 "Build Options" -> "Build Type" -> "Release"
```

## 运行程序

### 本地运行

```bash
# 运行应用程序
./bin/ccm_collector
./bin/ccm_logger
./bin/ccm_health

# 查看帮助
./bin/ccm_collector --help

# 指定配置文件
./bin/ccm_collector -c /etc/ccm/collector.conf
```

### 目标板运行

```bash
# 1. 复制文件到目标板
scp bin/* root@target:/usr/local/bin/
scp lib/*.so root@target:/usr/local/lib/

# 2. 更新动态库缓存（如果使用动态库）
ssh root@target ldconfig

# 3. 运行程序
ssh root@target /usr/local/bin/ccm_collector
```

## Buildroot 集成

EMS 完全兼容 Buildroot 构建系统：

### 创建 Buildroot Package

**推荐方式：使用 STAGING_DIR 环境变量**

```makefile
# package/ems/ems.mk
EMS_VERSION = 1.0.0
EMS_SITE = $(TOPDIR)/../EMS
EMS_SITE_METHOD = local
EMS_INSTALL_STAGING = YES

# 传递 STAGING_DIR 环境变量
EMS_MAKE_ENV = \
    STAGING_DIR=$(STAGING_DIR) \
    ARCH=$(KERNEL_ARCH) \
    CROSS_COMPILE=$(TARGET_CROSS)

define EMS_CONFIGURE_CMDS
    $(TARGET_MAKE_ENV) $(EMS_MAKE_ENV) $(MAKE) -C $(@D) \
        ccm_h200_am625_release_defconfig
endef

define EMS_BUILD_CMDS
    $(TARGET_MAKE_ENV) $(EMS_MAKE_ENV) $(MAKE) -C $(@D) \
        -j$(PARALLEL_JOBS)
endef

# 头文件和库已在 $(STAGING_DIR) 中，无需额外安装
define EMS_INSTALL_STAGING_CMDS
    # 可选：如果需要额外处理
endef

define EMS_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(STAGING_DIR)/bin/ccm_* $(TARGET_DIR)/usr/bin/
    $(INSTALL) -D -m 0755 $(STAGING_DIR)/lib/*.so $(TARGET_DIR)/usr/lib/
endef

$(eval $(generic-package))
```

**传统方式（不推荐）：**

```makefile
# 如果不使用 STAGING_DIR 环境变量
define EMS_BUILD_CMDS
    $(MAKE) -C $(@D) O=$(@D) $(TARGET_CONFIGURE_OPTS) \
        CROSS_COMPILE=$(TARGET_CROSS) \
        -j$(PARALLEL_JOBS)
endef

define EMS_INSTALL_STAGING_CMDS
    # 需要手动拷贝
    $(INSTALL) -D -m 0644 $(@D)/.staging/lib/*.a $(STAGING_DIR)/usr/lib/
    $(INSTALL) -D -m 0755 $(@D)/.staging/lib/*.so $(STAGING_DIR)/usr/lib/
    cp -r $(@D)/.staging/include/* $(STAGING_DIR)/usr/include/
endef

define EMS_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/.staging/bin/* $(TARGET_DIR)/usr/bin/
    $(INSTALL) -D -m 0755 $(@D)/lib/*.so $(TARGET_DIR)/usr/lib/
endef

$(eval $(generic-package))
```

### Buildroot 配置

```bash
# 在 Buildroot 中启用 EMS
make menuconfig
# 导航到 "Target packages" -> "EMS"

# 编译
make ems
```

## 常见问题

### 配置相关

**Q: 修改了 .config 但编译结果没变化？**

A: 需要同步配置：
```bash
make syncconfig
make clean && make
```

**Q: 如何恢复默认配置？**

A: 使用预定义配置：
```bash
make mrproper
make ccm_h200_am625_debug_defconfig
```

### 编译相关

**Q: 找不到头文件？**

A: 检查以下几点：
1. 库的 Makefile 中是否声明了 `header-y`
2. 应用程序 Makefile 中是否包含了 `-I$(objtree)/include`
3. 重新构建库以安装头文件：`make clean && make`

**Q: 并行编译失败？**

A: 检查依赖关系：
1. 确认顶层 Makefile 中声明了模块间依赖
2. 尝试单线程构建验证：`make -j1`
3. 检查是否有循环依赖

**Q: 库名重复前缀（liblibosal.a）？**

A: 这个问题已在最新版本中修复，两种写法都正确：
```makefile
lib-y += osal        # → libosal.a
lib-y += libosal.a   # → libosal.a
```

### 清理相关

**Q: make clean 后仍有残留文件？**

A: 使用更深层次的清理：
```bash
make mrproper    # 深度清理
make distclean   # 完全清理
```

**Q: 如何完全重置构建环境？**

A: 使用 git clean（谨慎使用）：
```bash
git clean -fdx   # 删除所有未跟踪的文件
```

### 交叉编译相关

**Q: 找不到交叉编译器？**

A: 确认工具链已安装并在 PATH 中：
```bash
which aarch64-linux-gnu-gcc
aarch64-linux-gnu-gcc --version
```

**Q: 交叉编译时链接错误？**

A: 检查以下几点：
1. 确认 `CONFIG_CROSS_COMPILE` 设置正确
2. 确认库文件是为目标架构编译的
3. 清理后重新构建：`make mrproper && make menuconfig && make`

## 性能优化

### 并行编译

```bash
# 使用所有 CPU 核心
make -j$(nproc)

# 限制并行任务数（避免内存不足）
make -j8
```

### 增量编译

构建系统自动支持增量编译：
- 只重新编译修改过的文件
- 使用 `.d` 文件跟踪头文件依赖
- 使用 `.cmd` 文件跟踪命令行变化

### ccache 加速

```bash
# 安装 ccache
sudo apt install ccache

# 使用 ccache
make CC="ccache gcc" -j$(nproc)

# 或在 menuconfig 中配置
# Build Options -> Use ccache
```

## 参考文档

- [README.md](../README.md) - 项目概述
- [BUILD_SYSTEM.md](BUILD_SYSTEM.md) - 构建系统详解
- [ARCHITECTURE.md](ARCHITECTURE.md) - 架构设计
- [PLATFORM.md](PLATFORM.md) - 平台配置指南
- [OSAL_PLATFORM.md](OSAL_PLATFORM.md) - OSAL 平台适配
- [HAL_PLATFORM.md](HAL_PLATFORM.md) - HAL 平台适配
- [STAGING_DIRECTORY.md](STAGING_DIRECTORY.md) - Staging 目录说明
- [DEFCONFIG_GUIDE.md](DEFCONFIG_GUIDE.md) - 配置文件指南
- [INSTALL.md](INSTALL.md) - 安装指南
- [CLAUDE.md](../CLAUDE.md) - AI 助手指南

---

**最后更新**: 2026-05-22  
**维护者**: wanguo  
**分支**: master
