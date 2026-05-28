# EMS SDK

EMS (Embedded Management System) 是一个采用 **Kconfig + CMake** 混合构建系统的嵌入式软件开发框架。

## ✨ 特性

- 🎯 **模块化架构**: Core/Products 分层设计，核心组件可复用
- ⚙️ **灵活配置**: Kconfig 图形化配置，支持功能裁剪
- 🚀 **快速构建**: CMake 构建系统，支持并行编译
- 📦 **多产品支持**: 一套代码支持多个产品变体
- 🔧 **跨平台**: 支持 Linux/RTOS/Bare-metal

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
# 加载示例配置
python3 build.py config sample_default

# 编译
python3 build.py build

# 运行
./_build/bin/app_demo
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
- 🐛 [故障排除](docs/TROUBLESHOOTING.md) - 常见问题解答

### 构建系统
- 🔨 [构建指南](docs/CMAKE_BUILD_GUIDE.md) - CMake 构建详解
- ⚙️ [配置指南](docs/CONFIGURATION.md) - Kconfig 配置说明
- 🔗 [Kconfig 集成](docs/CMAKE_KCONFIG_INTEGRATION.md) - 构建系统原理

### 项目指南
- 📋 [项目完整指南](CLAUDE.md) - AI 助手使用的项目上下文

## 📂 项目结构

```
EMS/
├── core/                     # 核心模块（可复用）
│   ├── osal/                # 操作系统抽象层
│   ├── hal/                 # 硬件抽象层
│   ├── pcl/                 # 外设配置层
│   ├── pdl/                 # 外设驱动层
│   └── acl/                 # 应用配置层
├── products/                 # 产品应用
│   ├── ccm/                 # CCM 产品（卫星载荷管理）
│   └── sample/              # 示例产品
├── examples/                 # 示例代码（即将添加）
├── tests/                    # 测试代码
├── configs/                  # 预定义配置
├── docs/                     # 文档
└── build.py                  # 统一构建脚本
```

## 🎯 可用配置

| 配置 | 产品 | 平台 | 用途 | 大小 |
|------|------|------|------|------|
| `sample_default` | Sample | x86_64 | 学习示例 | 824K |
| `ccm_minimal` | CCM | x86_64 | 最小配置 | 2.7M |
| `ccm_h200_100p_v1` | CCM | ARM64 | 基础型号 | 2.9M |
| `ccm_h200_100p_v2` | CCM | ARM64 | 增强型号 | 3.6M |
| `ccm_h200_200p` | CCM | ARM64 | 完整型号 | 4.1M |
| `ccm_2apps` | CCM | x86_64 | 功能裁剪示例 | 2.9M |

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
python3 build.py distclean

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

### PCL (外设配置层)
设备树风格的硬件配置管理，描述硬件拓扑和连接关系。

**内存占用**: ~10KB + 配置数据

### PDL (外设驱动层)
高层外设驱动：卫星通信、BMC 管理、MCU 通信、看门狗服务。

**内存占用**: ~8KB (单模块) ~ 40KB (全模块)

### ACL (应用配置层)
业务逻辑到硬件的映射层，提供应用级配置接口。

**内存占用**: ~20KB + 配置表

## 🎓 学习路径

### 第一步：运行示例
```bash
python3 build.py config sample_default
python3 build.py build
./_build/bin/app_demo
```

### 第二步：理解架构
阅读 [架构概述](docs/ARCHITECTURE.md)，了解模块关系和设计理念。

### 第三步：查看示例代码
浏览 `products/sample/` 和 `examples/`（即将添加），学习 API 使用。

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
- [开发者指南](docs/DEVELOPER_GUIDE.md)

提交 Pull Request 前请确保：
- ✅ 代码符合编码规范
- ✅ 所有配置编译通过
- ✅ 添加了必要的测试
- ✅ 更新了相关文档

## 📊 项目状态

- ✅ 核心架构稳定
- ✅ 构建系统完善
- ✅ 6 个配置全部测试通过
- ✅ 压力测试 100% 通过率
- 🚧 示例代码开发中
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

**提示**: 首次使用建议从 `sample_default` 配置开始，熟悉后再尝试其他配置。

**文档**: 完整文档请访问 [docs/](docs/) 目录。

**更新日志**: 查看 [Git 提交历史](https://github.com/wanguo99/EMS/commits/master)。
