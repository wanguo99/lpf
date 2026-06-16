# ES-Middleware SDK

ES-Middleware (Embedded Software - Middleware) 是一个采用 Kconfig + CMake 混合构建系统的嵌入式软件开发框架。

## 主要特性

- 模块化架构: Core/Products 分层设计，核心组件可复用
- 配置系统: Kconfig 图形化配置，支持功能裁剪
- 构建系统: CMake 构建，支持并行编译
- 多产品支持: 一套代码支持多个产品变体
- 跨平台: 支持 Linux/RTOS/Bare-metal
- 预定义配置: 提供开发、生产、测试等配置

## 快速开始

### 环境准备

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libncurses-dev

# macOS
brew install cmake ncurses
```

### 编译示例

```bash
# 查看可用配置
make list

# 加载配置并构建
make config CONFIG=tests_x86_minimal
make build

# 运行测试
./_build/bin/es-middleware-test --all
```

### 常用命令

```bash
make help              # 显示帮助信息
make list              # 列出所有配置
make menuconfig        # 图形化配置界面
make build             # 构建项目
make clean             # 清理构建产物
make distclean         # 完全清理
```

## 文档导航

### 核心文档
- [系统架构](docs/ARCHITECTURE.md) - 完整的系统架构说明（含架构图）
- [架构图集](docs/diagrams/) - 7 张高清架构图（PNG/SVG）
- [命名规范](docs/NAMING_CONVENTIONS.md) - 命名规范详解
- [PRL 架构设计](docs/PRL_ARCHITECTURE.md) - 协议层架构详解

### 构建系统
- [构建系统指南](docs/BUILD_SYSTEM_USER_GUIDE.md) - 完整的构建命令和工作流程
- [配置目录结构](docs/CONFIG_STRUCTURE.md) - 配置文件组织方式

### 设计文档
- [ES-IPC 设计](docs/ES-IPC_DESIGN.md) - 进程间通信设计
- [版本信息](docs/VERSION_INFO.md) - 版本管理说明

### 项目指南
- [项目完整指南](CLAUDE.md) - AI 助手使用的项目上下文
- [变更日志](CHANGELOG.md) - 项目更新记录

## 项目结构

```
ES-Middleware/
├── core/                     # 核心模块（可复用）
│   ├── osal/                # 操作系统抽象层
│   ├── hal/                 # 硬件抽象层
│   ├── pconfig/             # 平台配置层
│   ├── pdl/                 # 外设驱动层
│   ├── prl/                 # 协议层（统一设备协议）
│   └── aconfig/             # 应用配置层
├── products/                 # 产品应用
│   └── ccm/                 # CCM 产品（通信管理板）
│       ├── apps/            # 应用程序
│       ├── libs/            # 产品库
│       └── configs/         # 平台配置
├── configs/                  # 预定义配置
├── docs/                     # 文档
```

## 可用配置

### 产品配置（位于 configs/ccm/）

| 配置 | 平台 | 用途 |
|------|------|------|
| `ccm_h200_100p_am625_debug_defconfig` | ARM64 | CCM H200-100P-AM625 调试版本，包含所有调试功能 |
| `ccm_h200_100p_am625_release_defconfig` | ARM64 | CCM H200-100P-AM625 发布版本，优化配置 |

### 测试配置（位于 configs/tests/）

#### x86_64 测试配置

| 配置 | 测试范围 |
|------|---------|
| `tests_x86_full_defconfig` | 全栈测试（所有模块） |
| `tests_x86_minimal_defconfig` | 最小化配置（核心功能） |
| `tests_x86_osal_defconfig` | OSAL 单元测试 |
| `tests_x86_pdl_defconfig` | PDL 单元测试 |
| `tests_x86_prl_defconfig` | PRL 协议层测试 |
| `tests_x86_aconfig_defconfig` | ACONFIG 单元测试 |
| `tests_x86_pconfig_defconfig` | PCONFIG 单元测试 |
| `tests_x86_system_defconfig` | 系统集成测试 |
| `tests_x86_stress_defconfig` | 压力测试 |

#### ARM64 测试配置

| 配置 | 测试范围 |
|------|---------|
| `tests_arm64_full_defconfig` | 全栈测试（所有模块） |
| `tests_arm64_minimal_defconfig` | 最小化配置 |
| `tests_arm64_osal_defconfig` | OSAL 单元测试 |
| `tests_arm64_hal_defconfig` | HAL 单元测试 |
| `tests_arm64_pdl_defconfig` | PDL 单元测试 |
| `tests_arm64_prl_defconfig` | PRL 协议层测试 |
| `tests_arm64_aconfig_defconfig` | ACONFIG 单元测试 |
| `tests_arm64_pconfig_defconfig` | PCONFIG 单元测试 |
| `tests_arm64_system_defconfig` | 系统集成测试 |
| `tests_arm64_stress_defconfig` | 压力测试 |

## 常用命令

```bash
# 列出所有可用配置
make list

# 加载配置并构建
make tests_x86_minimal_defconfig
make

# 图形化配置（高级）
make menuconfig

# 安装
make install                    # 安装二进制和库
make install_headers            # 仅安装开发头文件（类似 kernel 的 headers_install）

# 安装选项
make install DESTDIR=/tmp/root CMAKE_INSTALL_PREFIX=/usr
make install_headers DESTDIR=/tmp/root CMAKE_INSTALL_PREFIX=/usr

# 清理
make clean       # 清理编译产物
make distclean   # 完全清理（包括配置）
```

### install_headers 说明

`make install_headers` 命令仅安装开发头文件，不需要先编译项目：

- **无需编译**：只需要加载配置（`make <config>_defconfig`）
- **按需安装**：仅安装已启用模块的头文件
- **支持 DESTDIR**：可以安装到临时目录进行打包
- **类似内核**：设计类似 Linux 内核的 `make headers_install`

**使用场景**：
- 为其他项目提供开发头文件
- 制作 SDK 开发包
- Buildroot/Yocto 集成时单独安装头文件

# 查看帮助
make help
```

## 测试框架

ES-Middleware 提供了一个统一的测试框架（Test Framework），支持单元测试、性能测试、压力测试和系统集成测试。

### 测试层次结构

测试框架采用四层层次结构，从高到低依次是：

#### 1. Layer（层）- 架构层次
对应项目的架构分层，例如：
- **OSAL** - 操作系统抽象层
- **HAL** - 硬件抽象层
- **PDL** - 外设驱动层
- **PRL** - 协议层
- **ACONFIG** - 应用配置层
- **PCONFIG** - 平台配置层
- **SYSTEM** - 系统集成层
- **PERFORMANCE** - 性能测试层

#### 2. Module（模块）- 功能模块
在某个 Layer 下的具体功能模块，例如在 OSAL 层下：
- `osal_mutex` - 互斥锁模块
- `osal_thread` - 线程模块
- `osal_semaphore` - 信号量模块
- `osal_clock` - 时钟模块

#### 3. Suite（测试套件）- 测试集合
针对某个模块的一组相关测试，通常一个模块对应一个测试套件。

#### 4. Test Case（测试用例）- 最小测试单元
具体的测试函数，每个测试用例测试一个特定的功能点。

### 测试命令

ES-Middleware 测试框架提供了丰富的命令行选项，参考了 pytest、cargo test、go test 等优秀测试框架的设计。

#### 运行测试

```bash
# 运行所有测试
./_build/bin/es-middleware-test --all
./_build/bin/es-middleware-test -a

# 按层运行测试
./_build/bin/es-middleware-test --layer OSAL
./_build/bin/es-middleware-test -L HAL

# 按模块运行测试
./_build/bin/es-middleware-test --module osal_mutex
./_build/bin/es-middleware-test -m osal_mutex

# 运行特定测试套件
./_build/bin/es-middleware-test --suite osal_mutex
./_build/bin/es-middleware-test -s osal_mutex

# 交互式菜单（默认）
./_build/bin/es-middleware-test
./_build/bin/es-middleware-test --interactive
```

#### 浏览测试结构

```bash
# 列出所有层（概览）
./_build/bin/es-middleware-test --list-layers

# 列出所有模块
./_build/bin/es-middleware-test --list-modules

# 列出特定层的模块
./_build/bin/es-middleware-test --list-modules --layer OSAL

# 列出所有测试套件
./_build/bin/es-middleware-test --list-suites

# 列出特定层的测试套件
./_build/bin/es-middleware-test --list-suites --layer OSAL

# 列出特定模块的测试套件
./_build/bin/es-middleware-test --list-suites --module osal_mutex

# 列出所有测试（含详细测试用例）
./_build/bin/es-middleware-test --list

# 列出特定层的详细测试
./_build/bin/es-middleware-test --list --layer OSAL

# 列出特定模块的详细测试
./_build/bin/es-middleware-test --list --module osal_mutex
```

#### 过滤测试

```bash
# 按测试类别过滤
./_build/bin/es-middleware-test --all --category unit
./_build/bin/es-middleware-test --all --category performance
./_build/bin/es-middleware-test --all --category stress
./_build/bin/es-middleware-test --all --category system

# 按执行速度过滤
./_build/bin/es-middleware-test --all --fast          # 快速测试 (<100ms)
./_build/bin/es-middleware-test --all --slow          # 慢速测试 (>1s)

# 按标签过滤
./_build/bin/es-middleware-test --all --tags fast,memory
./_build/bin/es-middleware-test --layer OSAL --tags hardware

# 排除特定类型的测试
./_build/bin/es-middleware-test --all --exclude-hardware
./_build/bin/es-middleware-test --all --exclude-network

# 组合过滤
./_build/bin/es-middleware-test --layer OSAL --fast --exclude-hardware
```

#### 输出格式

```bash
# 标准文本输出（默认）
./_build/bin/es-middleware-test --all

# JUnit XML 格式（用于 CI/CD）
./_build/bin/es-middleware-test --all --format junit --output report.xml

# JSON 格式
./_build/bin/es-middleware-test --all --format json --output results.json

# 禁用统计信息
./_build/bin/es-middleware-test --all --no-stats
```

#### 其他选项

```bash
# 查看版本信息
./_build/bin/es-middleware-test --version

# 查看帮助
./_build/bin/es-middleware-test --help

# 使用符号链接（按层过滤）
osal-test --all
hal-test --list-modules
pdl-test --suite pdl_mcu
```

### 测试统计示例

运行 `./_build/bin/es-middleware-test -l` 会显示完整的测试统计：

```
=====================================
Summary:
  Layers:      8
  Modules:     45
  Suites:      47
  Test cases:  544
=====================================
```

### 测试框架特性

ES-Middleware 测试框架参考了 pytest、cargo test、go test、ctest 等优秀开源项目的设计。

#### 核心特性

- ✅ **四层层次结构**: Layer → Module → Suite → Test Case
- ✅ **统计信息完整**: 每个列表命令都显示汇总统计
- ✅ **命令简洁直观**: 支持长短选项，易于记忆
- ✅ **灵活的过滤**: 按层、模块、类别、标签过滤测试
- ✅ **多种输出格式**: 支持 text、JUnit XML、JSON
- ✅ **交互式菜单**: 友好的图形化选择界面
- ✅ **符号链接支持**: osal-test、hal-test 等快捷方式
- ✅ **CI/CD 集成**: 完善的测试报告导出

#### 当前测试规模

| 层 | 模块数 | 套件数 | 测试用例数 |
|----|--------|--------|-----------|
| OSAL | 22 | 24 | 225 |
| HAL | 9 | 9 | 143 |
| PRL | 6 | 6 | 90 |
| ACONFIG | 4 | 4 | 34 |
| PCONFIG | 1 | 1 | 38 |
| 其他 | 3 | 3 | 14 |
| **总计** | **45** | **47** | **544** |

#### 测试类型

- **单元测试** (Unit Tests): 独立模块功能测试
- **性能测试** (Performance Tests): 性能基准测试
- **压力测试** (Stress Tests): 高负载和并发测试
- **系统测试** (System Tests): 端到端集成测试

#### 命令示例速查

```bash
# 浏览测试结构
es-middleware-test --list-layers                    # 查看所有层
es-middleware-test --list-modules --layer OSAL      # 查看OSAL模块
es-middleware-test --list-suites --module osal_mutex # 查看模块套件

# 运行测试
es-middleware-test --all                            # 运行全部
es-middleware-test --layer OSAL --fast              # OSAL快速测试
es-middleware-test --all --category unit            # 只运行单元测试

# CI/CD集成
es-middleware-test --all --format junit --output report.xml
```

## 核心模块

### OSAL (操作系统抽象层)
提供跨平台的操作系统接口：线程、互斥锁、信号量、文件、网络等。

**内存占用**: ~5KB (最小) ~ 50KB (完整)

### HAL (硬件抽象层)
提供统一的硬件驱动接口：CAN、UART、I2C、SPI、GPIO、Watchdog。

**内存占用**: ~5KB (单驱动) ~ 50KB (全驱动)

### PConfig (平台配置层)
设备树风格的硬件配置管理，描述硬件拓扑和连接关系。

**内存占用**: ~10KB + 配置数据

### PDL (外设驱动层)
高层外设驱动：MCU 通信、看门狗服务等。

**内存占用**: ~8KB (单模块) ~ 40KB (全模块)

### PRL (协议层)
统一设备协议，支持 MCU、CCM、PMC、GSC、POWER 等设备间通信。

**特点**：
- 统一协议格式（20 字节协议头）
- 按设备类型组织
- 零拷贝设计
- CRC16 校验

**内存占用**: ~15KB + 协议缓冲区

### AConfig (应用配置层)
业务逻辑到硬件的映射层，提供应用级配置接口。

**内存占用**: ~20KB + 配置表

## 学习路径

### 第一步：运行示例
```bash
make tests_x86_minimal_defconfig
make
./_build/bin/es-middleware-test
```

### 第二步：理解架构
阅读 [架构概述](docs/ARCHITECTURE.md)，了解模块关系和设计理念。

### 第三步：查看示例代码
浏览 `products/ccm/apps/`，学习 API 使用。

### 第四步：修改配置
```bash
make menuconfig  # 图形化配置
make             # 重新编译
```

### 第五步：添加新功能
参考 [CLAUDE.md](CLAUDE.md) 中的开发指南，添加自己的模块或应用。

## 技术支持

如需技术支持，请参考：
- [构建系统指南](docs/BUILD_SYSTEM_USER_GUIDE.md)
- [项目指南](CLAUDE.md) - 故障排除章节

## 版权声明

版权所有 © 2026 保留所有权利。

本软件及相关文档为专有软件产品。未经授权，禁止以任何形式复制、分发或修改本软件。

---

**使用建议**: 
- 首次使用建议从 `tests_x86_minimal_defconfig` 配置开始
- 开发和调试使用 `tests_x86_full_defconfig` 或 `ccm_h200_100p_am625_debug_defconfig`
- 生产部署使用 `ccm_h200_100p_am625_release_defconfig`

**文档**: 完整文档请访问 [docs/](docs/) 目录

**更新日志**: 查看 [CHANGELOG.md](CHANGELOG.md) 了解版本更新信息
