# PMC Product (通信管理板)

## 产品概述

PMC (Communication Control Module) 是通信管理板产品，负责系统内各设备间的通信协调和管理。

### 目标平台

- **H200-100P-AM625**: 基于 TI AM625 处理器的 H200 平台

## 快速开始

### 1. 编译 PMC 产品

```bash
# 调试版本（开发用）
make pmc_h200_100p_am625_debug_defconfig
make

# 发布版本（生产用）
make pmc_h200_100p_am625_release_defconfig
make
```

### 2. 运行应用

```bash
# 运行 PMC 应用（根据配置编译的应用）
./_build/bin/pmc_collector
./_build/bin/pmc_comm
./_build/bin/pmc_health
./_build/bin/pmc_logger
./_build/bin/pmc_supervisor
```

## 应用说明

PMC 产品包含以下应用程序：

- **pmc_collector**: 数据采集应用
- **pmc_comm**: 通信管理应用
- **pmc_health**: 健康监控应用
- **pmc_logger**: 日志管理应用
- **pmc_supervisor**: 系统监控应用

通过 Kconfig 配置可以选择性启用/禁用应用。

## 自定义配置

### 使用 menuconfig

```bash
# 加载基础配置
make pmc_h200_100p_am625_debug_defconfig

# 打开图形化配置界面
make menuconfig

# 导航到 "Products" -> "PMC Applications"
# 选择需要的应用

# 保存并编译
make
```

### 保存自定义配置

```bash
# 保存当前配置为新的 defconfig
make savedefconfig my_custom_pmc

# 配置文件保存到 configs/pmc/my_custom_pmc_defconfig
```

## 清理

```bash
# 清理构建产物
make clean

# 完全清理（包括配置）
make distclean
```

## 目录结构

```
products/pmc/
├── apps/                   # PMC 应用程序
│   ├── collector/         # 数据采集应用
│   ├── comm/              # 通信应用
│   ├── health/            # 健康监控应用
│   ├── logger/            # 日志应用
│   └── supervisor/        # 监控应用
├── libs/                   # PMC 产品库
│   └── libpmc/            # PMC 通用库
├── configs/                # 平台配置
│   └── h200_100p_am625/   # H200-100P-AM625 配置
├── osal/                   # PMC 特定的 OSAL 实现
├── hal/                    # PMC 特定的 HAL 实现
├── pdl/                    # PMC 特定的 PDL 实现
├── prl/                    # PMC 特定的 PRL 实现
└── pconfig/                # PMC 特定的 PCONFIG 实现
```

## 添加新应用

参考 [CLAUDE.md](../../CLAUDE.md) 中的"添加新的产品应用程序"章节。

## 参考文档

- [项目指南](../../CLAUDE.md)
- [系统架构](../../docs/ARCHITECTURE.md)
- [命名规范](../../docs/NAMING_CONVENTIONS.md)
