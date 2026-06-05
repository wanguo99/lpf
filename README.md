# EMS SDK

EMS (Embedded Management System) 是一个采用 **Kconfig + CMake** 混合构建系统的嵌入式软件开发框架。

## ✨ 特性

- 🎯 **模块化架构**: Core/Products 分层设计，核心组件可复用
- ⚙️ **灵活配置**: Kconfig 图形化配置，支持功能裁剪
- 🚀 **快速构建**: CMake 构建系统，支持并行编译
- 📦 **多产品支持**: 一套代码支持多个产品变体
- 🔧 **跨平台**: 支持 Linux/RTOS/Bare-metal
- 📋 **场景化配置**: 提供开发、生产、测试等多种预定义配置

## 🚀 快速开始

### 1. 环境准备

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake python3 libncurses-dev flex bison

# macOS
brew install cmake python3 ncurses flex bison
```

### 2. 获取代码

```bash
git clone https://github.com/wanguo99/EMS.git
cd EMS
```

### 3. 编译第一个示例

```bash
# 加载开发配置
python3 build.py config ccm_h200p100_development_defconfig

# 编译
python3 build.py build

# 运行
./_build/bin/collector
```

**就这么简单！** 🎉

## 📚 文档导航

### 新手入门
- 📖 [新手入门教程](docs/GETTING_STARTED.md) - **从零开始，5分钟上手**
- 🔧 [安装指南](docs/INSTALL.md) - 环境配置和依赖安装
- ⚡ [快速参考](docs/QUICK_START.md) - 常用命令速查

### 开发指南
- 🏗️ [架构概述](docs/ARCHITECTURE.md) - 系统架构和模块设计
- 👨‍💻 [开发者指南](docs/DEVELOPER_GUIDE.md) - 如何添加新功能
- 📝 [编码规范](docs/CODING_STANDARDS.md) - 代码风格和规范
- 📝 [命名规范](docs/NAMING_CONVENTIONS.md) - 命名规范详解
- 🐛 [故障排除](docs/TROUBLESHOOTING.md) - 常见问题解答

### 构建系统
- 🔨 [构建指南](docs/CMAKE_BUILD_GUIDE.md) - CMake 构建详解
- ⚙️ [配置指南](docs/CONFIGURATION.md) - Kconfig 配置说明
- 🔗 [Kconfig 集成](docs/CMAKE_KCONFIG_INTEGRATION.md) - 构建系统原理

### 协议层文档
- 🔌 [PRL 架构设计](docs/PRL_ARCHITECTURE.md) - 协议层架构详解
- 📖 [PRL 使用指南](docs/PRL_USAGE_GUIDE.md) - 协议层 API 使用

### 项目指南
- 📋 [项目完整指南](CLAUDE.md) - AI 助手使用的项目上下文

## 📂 项目结构

```
EMS/
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
└── build.py                  # 统一构建脚本
```

## 🎯 可用配置

### 产品配置

| 配置 | 场景 | 平台 | 用途 | 特点 |
|------|------|------|------|------|
| `ccm_h200p100_development_defconfig` | 开发 | x86_64 | CCM H200P100 开发环境 | 包含所有调试功能和测试工具 |
| `ccm_h200p100_production_defconfig` | 生产 | ARM64 | CCM H200P100 生产部署 | 优化的生产配置，禁用测试 |

### 测试配置

| 配置 | 测试范围 | 平台 | 用途 |
|------|---------|------|------|
| `tests_x86_full_defconfig` | 全栈测试 | x86_64 | 所有模块、所有功能 |
| `tests_x86_pdl_defconfig` | PDL 单元测试 | x86_64 | 仅测试 PDL 模块 |
| `tests_x86_prl_defconfig` | PRL 单元测试 | x86_64 | 仅测试 PRL 协议层 |
| `tests_x86_aconfig_defconfig` | ACONFIG 单元测试 | x86_64 | 仅测试 ACONFIG 模块 |
| `tests_x86_pconfig_defconfig` | PCONFIG 单元测试 | x86_64 | 仅测试 PCONFIG 模块 |
| `tests_x86_system_defconfig` | 系统测试 | x86_64 | 系统级集成测试 |
| `tests_x86_stress_defconfig` | 压力测试 | x86_64 | 性能和稳定性测试 |
| `tests_x86_minimal_defconfig` | 最小化配置 | x86_64 | 仅包含核心功能，适合资源受限环境 |

## 🔧 常用命令

```bash
# 列出所有可用配置
ls configs/*_defconfig

# 加载配置
python3 build.py config <config_name>

# 图形化配置（高级）
python3 build.py menuconfig

# 编译
python3 build.py build

# 清理
python3 build.py clean       # 清理编译产物
python3 build.py distclean   # 完全清理（包括配置）

# 查看帮助
python3 build.py --help
```

## 🏗️ 核心模块

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

## 🎓 学习路径

### 第一步：运行示例
```bash
python3 build.py config ccm_development
python3 build.py build
./_build/bin/collector
```

### 第二步：理解架构
阅读 [架构概述](docs/ARCHITECTURE.md)，了解模块关系和设计理念。

### 第三步：查看示例代码
浏览 `products/ccm/apps/`，学习 API 使用。

### 第四步：修改配置
```bash
python3 build.py menuconfig  # 图形化配置
python3 build.py build       # 重新编译
```

### 第五步：添加新功能
参考 [开发者指南](docs/DEVELOPER_GUIDE.md)，添加自己的模块或应用。

## 🤝 贡献代码

欢迎贡献！请参考：
- [编码规范](docs/CODING_STANDARDS.md)
- [命名规范](docs/NAMING_CONVENTIONS.md)
- [开发者指南](docs/DEVELOPER_GUIDE.md)

提交 Pull Request 前请确保：
- ✅ 代码符合编码规范和命名规范
- ✅ 所有配置编译通过
- ✅ 添加了必要的测试
- ✅ 更新了相关文档

## 📊 项目状态

- ✅ 核心架构稳定
- ✅ 构建系统完善
- ✅ 14 个配置全部测试通过
- ✅ 压力测试 100% 通过率
- ✅ PRL 协议层重构完成（v1.2）
- ✅ 命名规范统一完成
- 🚧 API 文档生成中

## 🐛 问题反馈

遇到问题？
1. 查看 [故障排除指南](docs/TROUBLESHOOTING.md)
2. 搜索 [Issues](https://github.com/wanguo99/EMS/issues)
3. 提交新的 Issue

## 📄 许可证

[MIT License](LICENSE)

## 🙏 致谢

感谢所有贡献者！

---

**提示**: 
- 首次使用建议从 `ccm_development` 配置开始
- 单元测试使用对应的 `*_test` 配置
- 生产部署使用 `ccm_production` 配置

**文档**: 完整文档请访问 [docs/](docs/) 目录

**更新日志**: 查看 [Git 提交历史](https://github.com/wanguo99/EMS/commits/master)
