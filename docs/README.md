# EMS 项目文档

欢迎使用 EMS (Embedded Management System) 项目文档。

## 快速导航

### 新手入门

1. **[安装指南](INSTALL.md)** - 安装依赖和环境配置
2. **[构建指南](BUILD.md)** - 编译和构建项目
3. **[配置指南](CONFIGURATION.md)** - 配置系统和模块

### 开发参考

4. **[架构概述](ARCHITECTURE.md)** - 系统架构和模块设计
5. **[编码规范](CODING_STANDARDS.md)** - 代码风格和开发规范

## 文档说明

本文档集提供 EMS 项目的核心使用和开发指南：

- **INSTALL.md** - 系统要求、依赖安装、编译安装、故障排除
- **BUILD.md** - 构建命令、配置系统、交叉编译、测试运行
- **CONFIGURATION.md** - 平台配置、模块裁剪、构建选项、配置管理
- **ARCHITECTURE.md** - 分层架构、核心模块、依赖关系、扩展方法
- **CODING_STANDARDS.md** - C 语言规范、Makefile 规范、Git 提交规范

## 快速开始

```bash
# 1. 安装依赖
sudo apt-get install build-essential gcc make bison flex libncurses-dev

# 2. 获取源码
git clone https://github.com/your-org/EMS.git
cd EMS

# 3. 配置
make x86_64_full_defconfig

# 4. 编译
make -j$(nproc)

# 5. 运行测试
export LD_LIBRARY_PATH=$(pwd)/.staging/lib
./.staging/bin/ems-unit-test
```

## 获取帮助

- **问题反馈**: 提交 Issue 到项目仓库
- **功能建议**: 提交 Feature Request
- **贡献代码**: 参考 CODING_STANDARDS.md

## 项目信息

- **项目主页**: https://github.com/your-org/EMS
- **许可证**: [LICENSE](../LICENSE)
- **版本**: 1.0.0
