# EMS (Embedded Management System)

嵌入式管理系统 - 基于 Linux Kbuild 构建框架的模块化嵌入式软件平台。

## 项目概述

EMS 是一个采用 Linux 内核风格构建系统的嵌入式软件项目，提供了完整的硬件抽象层（HAL）、操作系统抽象层（OSAL）和产品应用框架。

### 核心特性

- **Kbuild 构建系统**: 采用 Linux 内核风格的构建框架，支持配置管理、条件编译、并行构建
- **Kconfig 配置管理**: 图形化配置界面（menuconfig），支持依赖检查和配置验证
- **模块化架构**: Core/Products 分层设计，清晰的模块边界和依赖关系
- **Staging 头文件机制**: 自动化的头文件安装和管理，支持外部构建
- **标准化输出**: 遵循 Linux 标准目录结构（bin/、lib/、lib/modules/）

## 快速开始

### 环境要求

- Linux 操作系统（推荐 Ubuntu 20.04+）
- GCC 工具链
- Make 4.0+
- ncurses 开发库（用于 menuconfig）

### 构建步骤

```bash
# 1. 配置项目
make ccm_h200_am625_debug_defconfig

# 或使用图形化配置
make menuconfig

# 2. 编译项目（并行编译）
make -j$(nproc)

# 3. 查看输出
ls bin/      # 可执行文件
ls lib/      # 静态库和动态库
ls lib/modules/  # 内核模块

# 4. 清理
make clean      # 清理编译产物
make mrproper   # 深度清理（包括配置文件）
```

### 使用 PRODUCT 变量快速构建

```bash
# 一步完成配置和构建
make PRODUCT=ccm_h200_am625_debug -j$(nproc)
```

## 项目结构

```
EMS/
├── core/                   # 核心模块
│   ├── acl/               # 访问控制层
│   ├── hal/               # 硬件抽象层
│   ├── osal/              # 操作系统抽象层
│   ├── pcl/               # 协议控制层
│   └── pdl/               # 协议数据层
├── products/              # 产品应用
│   └── ccm/              # CCM 产品
│       ├── apps/         # 应用程序
│       └── libs/         # 产品库
├── configs/               # 配置文件
├── scripts/               # 构建脚本
│   ├── Makefile.build    # 核心构建规则
│   ├── Makefile.clean    # 清理规则
│   └── kconfig/          # Kconfig 工具
├── include/               # 公共头文件（构建时生成）
├── bin/                   # 可执行文件输出（构建时生成）
├── lib/                   # 库文件输出（构建时生成）
└── docs/                  # 项目文档
```

## 构建系统特性

### 1. 声明式 Makefile

采用 Linux 内核风格的声明式语法：

```makefile
# 应用程序
app-y := ccm_collector

# 静态库
lib-y := osal

# 动态库
so-y := libccm

# 内核模块
obj-m := driver.o

# 复合对象
ccm_collector-objs := main.o config.o logger.o
```

### 2. Staging 头文件机制

库的头文件自动安装到 staging 目录，应用程序无需硬编码源码路径：

```makefile
# 库 Makefile
lib-y := osal
header-y := osal.h osal_types.h
header-y += ipc/osal_mutex.h

# 应用程序 Makefile
app-y := myapp
ccflags-y += -I$(objtree)/include  # 自动包含所有库头文件
```

### 3. 智能库名处理

自动检测库名前缀，避免重复添加：

```makefile
lib-y += osal        # 输出: libosal.a
lib-y += libosal.a   # 输出: libosal.a（不会变成 liblibosal.a）
```

### 4. 条件编译

支持 Kconfig 配置驱动的条件编译：

```makefile
obj-$(CONFIG_OSAL_NETWORK) += osal_socket.o
obj-$(CONFIG_HAL_CAN) += hal_can_linux.o
subdir-$(CONFIG_CCM_APPS) += apps/
```

### 5. 外部构建支持

支持源码和构建产物分离：

```bash
make O=/path/to/build ccm_h200_am625_debug_defconfig
make O=/path/to/build -j$(nproc)
```

## 配置管理

### 可用配置

```bash
make help                              # 查看所有可用配置
make ccm_h200_am625_debug_defconfig   # CCM 调试配置
make ccm_h200_am625_release_defconfig # CCM 发布配置
```

### 配置工具

```bash
make menuconfig    # 图形化配置（推荐）
make defconfig     # 使用默认配置
make savedefconfig # 保存最小化配置
```

## 清理目标

```bash
make clean         # 清理编译产物（bin/、lib/、*.o、*.a、*.so）
make mrproper      # 深度清理（包括 .config、kconfig 工具、staging 头文件）
make distclean     # 完全清理（包括编辑器备份文件等）
```

## 开发指南

### 添加新的应用程序

1. 在 `products/ccm/apps/` 下创建目录
2. 创建 Makefile：

```makefile
app-y := myapp
myapp-objs := main.o module1.o module2.o

ccflags-y += -I$(objtree)/include
ccflags-y += -I$(src)/include
```

3. 在父目录 Makefile 中添加：`subdir-y += myapp`

### 添加新的库

1. 在 `core/` 下创建目录
2. 创建 Makefile：

```makefile
lib-y := mylib
mylib-objs := file1.o file2.o

# 导出头文件
header-y := mylib.h mylib_types.h
header-y += subdir/mylib_api.h

ccflags-y += -I$(src)/include
```

3. 头文件放在 `include/` 子目录下
4. 在父目录 Makefile 中添加：`subdir-y += mylib`

### 编码规范

- 遵循 Linux 内核编码风格
- 使用 Tab 缩进（8 空格宽度）
- 函数名使用小写加下划线
- 详见 `docs/CODING_STANDARDS.md`

## 文档

- [构建系统详解](docs/BUILD_SYSTEM.md) - 构建系统架构和原理
- [构建指南](docs/BUILD_GUIDE.md) - 详细的构建步骤和选项
- [架构设计](docs/ARCHITECTURE.md) - 系统架构和模块设计
- [平台配置指南](docs/PLATFORM.md) - 平台和架构配置说明
- [OSAL 平台适配](docs/OSAL_PLATFORM.md) - OSAL 多平台支持
- [Staging 目录说明](docs/STAGING_DIRECTORY.md) - 构建产物管理
- [配置文件指南](docs/DEFCONFIG_GUIDE.md) - 预定义配置说明
- [安装指南](docs/INSTALL.md) - 文件安装和部署
- [编码规范](docs/CODING_STANDARDS.md) - 代码质量和风格标准
- [Makefile 框架](docs/MAKEFILE_FRAMEWORK.md) - Makefile 框架使用指南

## 故障排除

### 构建失败

```bash
# 清理后重新构建
make mrproper
make ccm_h200_am625_debug_defconfig
make -j$(nproc)
```

### 配置问题

```bash
# 检查配置
cat .config

# 重新配置
make menuconfig
```

### 依赖问题

```bash
# 查看详细构建过程
make V=1

# 查看依赖关系
make -n
```

## 许可证

[待添加许可证信息]

## 贡献

[待添加贡献指南]

## 联系方式

[待添加联系方式]
