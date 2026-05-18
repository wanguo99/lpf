# EMS - Embedded Middleware System

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)]()
[![Architecture](https://img.shields.io/badge/arch-x86__64%20%7C%20ARM32%20%7C%20ARM64%20%7C%20RISC--V64-orange.svg)]()

通用嵌入式中间件系统，提供3层调用链(Apps→PDL→HAL)运行在OSAL运行环境之上，配合2个配置库(ACL/PCL)，连接硬件与应用，支持多种外设和通信协议。

## 概述

EMS 是一个模块化、可移植的嵌入式中间件框架，采用分层架构设计，从操作系统抽象到应用层提供完整的软件栈。适用于需要管理多种外设、支持多种通信协议的嵌入式控制系统。

**核心特性**：
- 🏗️ **清晰架构**：3层调用链(Apps→PDL→HAL) + OSAL运行环境 + 2个配置库(ACL/PCL)，职责清晰
- 🔧 **跨平台设计**：所有层使用OSAL接口，POSIX实现，易于移植到RTOS
- 🖥️ **多架构支持**：x86_64、ARM32、ARM64、RISC-V 64
- 🔌 **丰富驱动**：CAN、串口、I2C、SPI、GPIO、Watchdog等常用总线和外设
- ✅ **完整测试**：406测试用例，覆盖核心功能
- 📚 **详细文档**：每个模块都有完整的架构、API、使用文档

## 快速开始

### 编译

**方式一：标准 CMake 命令（推荐）**

```bash
# 本地编译
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Debug 模式
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# 交叉编译
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm32-linux-gnueabihf.cmake
cmake --build build -j$(nproc)
```

**方式二：build.sh 脚本（便捷）**

```bash
./build.sh              # Release模式
./build.sh -d           # Debug模式
./build.sh -c           # 清理
./build.sh -a arm32     # ARM32 交叉编译
./build.sh -a arm64     # ARM64 交叉编译
./build.sh -a riscv64   # RISC-V 64 交叉编译
```

详细编译指南：[docs/BUILD_GUIDE.md](docs/BUILD_GUIDE.md)

**安装交叉编译工具链：**

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-linux-gnueabihf      # ARM32
sudo apt-get install gcc-aarch64-linux-gnu        # ARM64
sudo apt-get install gcc-riscv64-linux-gnu        # RISC-V 64
```

### 安装

```bash
# 安装到默认路径 /usr/local
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build

# 安装到自定义路径
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/ems
cmake --build build -j$(nproc)
sudo cmake --install build
```

### 运行

```bash
# 运行示例应用
./build/bin/sample_app

# 运行看门狗应用
./build/bin/watchdog_app

# 运行测试
./build/bin/ems-test -i     # 交互式菜单
./build/bin/ems-test -a     # 运行所有测试
./build/bin/osal-test -a    # 仅运行OSAL层测试
```

### Buildroot 集成

EMS 完全兼容 Buildroot 构建系统：

```bash
# 1. 复制配置文件到 Buildroot
cp -r docs/buildroot/* <buildroot>/package/ems/

# 2. 在 menuconfig 中启用 EMS
make menuconfig
# Target packages -> Libraries -> ems

# 3. 构建
make ems
```

详细集成指南：[docs/buildroot/README.md](docs/buildroot/README.md)

## 架构设计

EMS采用清晰的分层架构：

```
┌─────────────────────────────────────────────────┐
│         OSAL Runtime Environment                │
│  (All layers use OSAL interfaces)               │
└─────────────────────────────────────────────────┘
      ↑            ↑            ↑
      │            │            │
   Apps ──────> PDL ──────> HAL
   (读取ACL)    (读取PCL)
```

**调用链**：Apps → PDL → HAL  
**运行环境**：所有层使用OSAL接口  
**配置库**：ACL(Apps读取)、PCL(PDL读取)

## 模块组成

### OSAL - 操作系统抽象层（运行环境）
为所有层提供统一的运行环境，封装所有系统调用和标准库函数。**所有层(Apps/PDL/HAL)都必须使用OSAL接口，禁止直接调用系统调用或标准库。**

**特性**：用户态库设计、线程安全、优雅关闭、死锁检测、日志轮转

**提供接口**：任务管理、队列、互斥锁、日志、文件I/O、网络、时间服务、内存操作、字符串操作

**文档**: [core/osal/README.md](core/osal/README.md) | [详细文档](core/osal/docs/)

### HAL - 硬件抽象层
硬件驱动封装，提供CAN、串口、I2C、SPI等硬件接口。**必须使用OSAL接口，禁止直接系统调用。**

**特性**：平台隔离、统一接口、驱动封装、配置管理

**支持硬件**：CAN总线、串口(UART)、I2C总线、SPI总线、GPIO、Watchdog

**文档**: [core/hal/README.md](core/hal/README.md) | [详细文档](core/hal/docs/)

### PDL - 外设驱动层
统一管理外部系统、BMC设备、MCU等外设服务，对应用提供访问外设的统一接口。**必须使用OSAL接口，读取PCL配置。**

**特性**：统一外设管理、多通道冗余、自动故障切换、心跳机制

**文档**: [core/pdl/README.md](core/pdl/README.md) | [详细文档](core/pdl/docs/)

### ACL - 应用配置层（配置库）
业务功能到设备的映射配置，被Apps层读取。**不参与调用链，纯配置数据。**

**特性**：O(1)查找、业务解耦、配置驱动

**文档**: [core/acl/README.md](core/acl/README.md)

### PCL - 外设配置库（配置库）
参考设备树架构，以外设为单位的硬件配置库，被PDL层读取。**不参与调用链，纯配置数据。**

**特性**：外设为单位、配置与代码分离、接口内嵌、运行时查询

**文档**: [core/pcl/README.md](core/pcl/README.md) | [详细文档](core/pcl/docs/)

### Apps - 应用层
业务应用层，读取ACL配置，调用PDL接口。**必须使用OSAL接口。**

**特性**：平台无关、使用抽象接口、优雅退出、错误处理

**文档**: [apps/README.md](apps/README.md) | [详细文档](apps/docs/)

### Tests - 测试框架
统一测试框架，提供跨层级的单元测试和集成测试能力。

**特性**：统一框架、交互式菜单、命令行模式、自动注册、详细报告

**文档**: [tests/README.md](tests/README.md)

## 目录结构

```
ems/
├── core/                   # 框架核心模块
│   ├── osal/              # 操作系统抽象层
│   │   ├── include/       # 公共头文件
│   │   ├── src/posix/     # POSIX实现
│   │   └── docs/          # 文档
│   ├── hal/               # 硬件抽象层
│   │   ├── include/       # 驱动接口
│   │   ├── src/linux/     # Linux实现
│   │   └── docs/          # 文档
│   ├── pdl/               # 外设驱动层
│   │   ├── include/       # 服务接口
│   │   ├── src/           # 服务实现
│   │   └── docs/          # 文档
│   ├── pcl/               # 硬件配置库
│   │   ├── include/       # 配置接口
│   │   ├── platform/      # 平台配置
│   │   └── docs/          # 文档
│   └── acl/               # 应用配置层
│       ├── include/       # 配置接口
│       └── docs/          # 文档
├── projects/               # 产品实现
│   ├── pmc_h200/          # PMC H200项目
│   └── samples/           # 示例项目
├── apps/                   # 应用层
│   ├── sample_app/        # 示例应用
│   └── docs/              # 文档
├── tests/                  # 测试框架
│   ├── include/           # 测试框架头文件
│   ├── core/              # 测试框架核心
│   ├── unit/              # 单元测试
│   │   ├── osal/          # OSAL层单元测试
│   │   ├── hal/           # HAL层单元测试
│   │   ├── pcl/           # PCL层单元测试
│   │   └── pdl/           # PDL层单元测试
│   ├── system/            # 系统测试
│   ├── stress/            # 压力测试
│   └── docs/              # 文档
├── output/                 # 编译输出
│   ├── build/             # 中间文件
│   └── target/            # 最终产物
└── docs/                   # 项目文档
    ├── ARCHITECTURE.md    # 架构设计
    ├── CODING_STANDARDS.md # 编码规范
    └── CLAUDE.md          # 开发指南
```

## 技术亮点

### 多架构支持
- 支持 x86_64、ARM32 (ARMv7-A)、ARM64 (ARMv8-A)、RISC-V 64
- 固定大小类型（`int32`、`uint64`）保证跨架构一致性
- 自动字节序检测和转换（`OSAL_HTONS/HTONL`）
- C11 原子操作支持所有架构

### 测试覆盖
- **406测试用例**覆盖核心功能
  - OSAL层：200测试用例（19模块：任务、队列、互斥锁、日志、文件、网络、时间等）
  - HAL层：89测试用例（6模块：CAN、串口、I2C、SPI、GPIO、Watchdog）
  - PCL层：22测试用例（配置管理）
  - PDL层：95测试用例（外设服务：Satellite、BMC、MCU、Watchdog）
- 交互式测试菜单和命令行模式
- 详细的测试报告和错误诊断

### 代码质量
- 遵循 MISRA C 编码规范
- 完整的错误处理和资源管理
- 线程安全设计
- 详细的 API 文档和使用示例

## 文档导航

### 模块文档
- [OSAL层](core/osal/README.md) - 操作系统抽象层（任务、队列、互斥锁、日志等）
- [HAL层](core/hal/README.md) - 硬件抽象层（CAN、串口等驱动）
- [PDL层](core/pdl/README.md) - 外设驱动层（外部系统、BMC设备、MCU服务）
- [PCL层](core/pcl/README.md) - 硬件配置库（设备树式配置管理）
- [Apps层](apps/README.md) - 应用层（示例应用）
- [Tests层](tests/README.md) - 测试框架（70+测试用例）

### 详细文档
- [OSAL详细文档](core/osal/docs/) - API参考、设计文档
- [HAL详细文档](core/hal/docs/) - 驱动开发、移植指南
- [PDL详细文档](core/pdl/docs/) - 服务设计、协议文档
- [PCL详细文档](core/pcl/docs/) - 配置规范、平台适配
- [Apps详细文档](apps/docs/) - 应用开发指南

### 项目文档
- [架构设计](docs/ARCHITECTURE.md) - 系统架构和设计理念
- [编码规范](docs/CODING_STANDARDS.md) - 代码风格和规范
- [构建系统](docs/BUILD_SYSTEM.md) - 构建系统详解
- [快速构建](docs/QUICK_BUILD.md) - 快速构建指南
- [开发指南](CLAUDE.md) - AI辅助开发指南
- [贡献指南](CONTRIBUTING.md) - 如何贡献代码
- [更新日志](CHANGELOG.md) - 版本历史

## 测试

```bash
# 交互式测试菜单
./build/bin/ems-test -i

# 运行所有测试
./build/bin/ems-test -a

# 运行指定层测试
./build/bin/ems-test -L OSAL
./build/bin/ems-test -L HAL

# 运行指定模块测试
./build/bin/ems-test -m test_osal_task
./build/bin/ems-test -m test_hal_can

# Busybox 风格快捷方式
./build/bin/osal-test -a    # 仅运行 OSAL 层测试
./build/bin/hal-test -a     # 仅运行 HAL 层测试
```

**测试覆盖**：
- OSAL层：19模块，200测试用例
- HAL层：6模块，89测试用例（CAN、串口、I2C、SPI、GPIO、Watchdog）
- PCL层：22测试用例
- PDL层：95测试用例（Satellite、BMC、MCU、Watchdog）
- 总计：406测试用例

## 开发环境

- **操作系统**：Linux (Ubuntu 20.04+)
- **编译器**：GCC 9.0+
- **构建工具**：CMake 3.16+
- **依赖库**：pthread, rt

## 代码规模

- 总代码量：约23,000行
- 生产代码：约13,000行
- 测试代码：约10,000行
- 代码文件：151个（.c/.h）

## 设计原则

1. **平台无关性**：OSAL是唯一允许包含系统头文件的层
2. **分层隔离**：上层只能调用下层接口，不能跨层调用
3. **配置分离**：硬件配置集中在PCL层
4. **错误处理**：所有函数返回int32状态码
5. **资源管理**：所有资源必须显式释放

## 许可证

本项目采用 GNU General Public License v3.0 许可证。详见 [LICENSE](LICENSE) 文件。

## 贡献

欢迎贡献代码和反馈！请查看 [CONTRIBUTING.md](CONTRIBUTING.md) 了解详细的贡献指南。

简要流程：
1. Fork 项目并创建特性分支
2. 遵循 [编码规范](docs/CODING_STANDARDS.md)
3. 添加测试并确保所有测试通过
4. 更新相关文档
5. 提交 Pull Request

## 更新日志

查看 [CHANGELOG.md](CHANGELOG.md) 了解版本历史和更新内容。

## 许可证

本项目采用 GNU General Public License v3.0 许可证。详见 [LICENSE](LICENSE) 文件。

## 联系方式

- 问题反馈：请通过 GitHub Issues 提交
- 代码贡献：请通过 Pull Request 提交
- 文档改进：欢迎提交文档相关的 PR
