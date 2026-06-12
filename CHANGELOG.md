# Changelog

All notable changes to ES-Middleware SDK will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Planned
- API 文档生成（Doxygen）
- 更多示例代码
- CI/CD 集成
- 性能优化工具

---

## [1.0.0] - 2026-05-28

### 🎉 首次正式发布

这是 ES-Middleware SDK 的第一个正式版本，包含完整的核心功能和文档。

### Added - 新增功能

#### 核心模块
- **OSAL (操作系统抽象层)**
  - 线程管理（创建、销毁、同步）
  - IPC 支持（互斥锁、信号量、共享内存）
  - 文件操作（POSIX 接口）
  - 网络通信（Socket、TCP/UDP）
  - 时间管理（时钟、定时器、延时）
  - 支持 Linux/POSIX 平台

- **HAL (硬件抽象层)**
  - CAN 总线驱动（Linux SocketCAN）
  - UART 串口驱动（/dev/tty*）
  - I2C 总线驱动（/dev/i2c-*）
  - SPI 总线驱动（/dev/spidev*）
  - GPIO 驱动（sysfs）
  - Watchdog 驱动（/dev/watchdog）
  - 支持 Linux 和 macOS 平台

- **PConfig (外设配置层)**
  - 设备树风格的硬件配置管理
  - 硬件拓扑描述
  - 平台配置支持（H200-100P V1/V2, H200-200P）

- **PDL (外设驱动层)**
  - 卫星通信协议（Satellite）
  - BMC 管理协议（IPMI/Redfish）
  - MCU 通信协议（CAN/UART）
  - Watchdog 服务

- **AConfig (应用配置层)**
  - 遥测数据管理
  - 遥控指令处理
  - 配置表管理
  - 遥测缓存

#### 产品应用
- **CCM 产品**
  - Collector 应用（数据采集）
  - Comm 应用（通信管理）
  - Health 应用（健康监控）
  - Logger 应用（日志记录）
  - Supervisor 应用（监控管理）

- **Sample 产品**
  - Demo 应用（示例演示）

#### 构建系统
- Config.in 配置系统（menuconfig 图形化配置）
- CMake 构建系统（3.16+）
- Python 构建脚本（build.py）
- 6 个预定义配置文件
- 支持静态库和动态库
- 支持交叉编译

#### 文档
- README.md（快速开始）
- GETTING_STARTED.md（新手入门教程）
- DEVELOPER_GUIDE.md（开发者指南）
- TROUBLESHOOTING.md（故障排除指南）
- ARCHITECTURE.md（架构文档，含 Mermaid 图）
- CMAKE_BUILD_GUIDE.md（构建指南）
- CONFIGURATION.md（配置指南）
- CODING_STANDARDS.md（编码规范）

#### 示例代码
- 01_hello_world（Hello World 示例）
- 02_osal_thread（线程和同步示例）
- 03_hal_can（CAN 总线通信示例）
- 04_hal_uart（串口通信示例）

#### 测试
- 单元测试框架
- 性能测试
- 压力测试
- 系统集成测试
- 53 个测试用例

### Changed - 变更

#### 构建系统优化
- 删除 include 目录软连接，统一头文件引用方式
- 重命名平台配置（`CONFIG_PLATFORM_GENERIC_LINUX` → `CONFIG_PLATFORM_LINUX`）
- 优化 CMakeLists.txt，使用显式文件列表替代 GLOB
- 修复配置文件问题，确保所有配置可用

#### 目录结构调整
- 重命名 HAL 目录（`generic-linux` → `linux`）
- 统一 PDL 目录结构（所有外设放在独立目录）
- 删除冗余的 Kconfig 子文件

#### 文档改进
- 修复 README.md 中的错误命令
- 统一所有文档使用 `python3 build.py` 命令
- 添加架构图和流程图（Mermaid）
- 完善 API 说明和使用示例

### Fixed - 修复

- 修复 `sample_default_defconfig` 缺少平台配置
- 修复 `ccm_2apps_defconfig` 重复的 `CONFIG_PLATFORM_LINUX`
- 修复文档与实际代码不一致的问题
- 修复 HAL 平台配置错误导致编译失败

### Removed - 删除

- 删除 5 个软连接（`core/{hal,osal,acl,pdl,pcl}/include/{模块名}`）
- 删除 AConfig API v2 版本（未使用的测试版本）
- 删除 14 个未使用的 Config.in 文件
- 删除冗余的备份文件

### Performance - 性能

- 构建系统优化，平均编译时间 1.02 秒
- 压力测试 100% 通过率（60 次测试）
- 支持并行编译

### Security - 安全

- 统一错误处理机制
- 输入验证和边界检查
- 资源泄漏检测

---

## [0.9.0] - 2026-05-20

### Added
- 初始项目结构
- 核心模块基础实现
- 基本构建系统

### Changed
- 项目重构为分层架构

---

## 版本说明

### 版本号规则

遵循语义化版本规范 (Semantic Versioning):

- **主版本号 (MAJOR)**: 不兼容的 API 变更
- **次版本号 (MINOR)**: 向后兼容的功能新增
- **修订号 (PATCH)**: 向后兼容的问题修复

### 发布周期

- **主版本**: 每年 1-2 次
- **次版本**: 每季度 1 次
- **修订版本**: 按需发布

### 支持策略

- **当前版本**: 完全支持
- **前一版本**: 安全更新和关键修复
- **更早版本**: 不再支持

---

## 贡献指南

如何为 CHANGELOG 做贡献：

1. **格式**: 遵循 [Keep a Changelog](https://keepachangelog.com/) 格式
2. **分类**: 使用标准分类（Added, Changed, Fixed, Removed, etc.）
3. **描述**: 清晰描述变更内容和影响
4. **链接**: 引用相关 Issue 或 PR

### 变更类型

- **Added**: 新功能
- **Changed**: 现有功能的变更
- **Deprecated**: 即将废弃的功能
- **Removed**: 已删除的功能
- **Fixed**: Bug 修复
- **Security**: 安全相关的修复
- **Performance**: 性能改进

---

## 链接

- [项目主页](https://github.com/wanguo99/ES-Middleware)
- [问题反馈](https://github.com/wanguo99/ES-Middleware/issues)
- [发布页面](https://github.com/wanguo99/ES-Middleware/releases)
- [文档](https://github.com/wanguo99/ES-Middleware/tree/master/docs)

---

**维护者**: wanguo  
**最后更新**: 2026-05-28
