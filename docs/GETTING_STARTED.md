# EMS SDK 新手入门教程

欢迎使用 EMS SDK！本教程将带你从零开始，在 **15 分钟内**完成第一个嵌入式应用的开发。

---

## 📋 目录

1. [环境准备](#1-环境准备)
2. [获取代码](#2-获取代码)
3. [编译第一个示例](#3-编译第一个示例)
4. [理解项目结构](#4-理解项目结构)
5. [修改示例代码](#5-修改示例代码)
6. [配置功能模块](#6-配置功能模块)
7. [添加新应用](#7-添加新应用)
8. [下一步](#8-下一步)

---

## 1. 环境准备

### 1.1 系统要求

- **操作系统**: Linux (Ubuntu 20.04+) 或 macOS
- **内存**: 至少 2GB
- **磁盘**: 至少 500MB 可用空间

### 1.2 安装依赖

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    python3 \
    libncurses-dev \
    flex \
    bison \
    git
```

#### macOS

```bash
brew install cmake python3 ncurses flex bison
```

### 1.3 验证安装

```bash
# 检查工具版本
gcc --version        # 应该 >= 7.0
cmake --version      # 应该 >= 3.16
python3 --version    # 应该 >= 3.6
```

✅ **检查点**: 所有命令都能正常运行，没有报错。

---

## 2. 获取代码

### 2.1 克隆仓库

```bash
# 克隆代码
git clone https://github.com/wanguo99/EMS.git
cd EMS

# 查看项目结构
ls -la
```

你应该看到：
```
core/       # 核心模块
products/   # 产品应用
configs/    # 配置文件
docs/       # 文档
build.py    # 构建脚本
```

### 2.2 了解构建脚本

```bash
# 查看帮助
python3 build.py --help
```

主要命令：
- `config <name>` - 加载配置
- `build` - 编译项目
- `distclean` - 清理构建
- `menuconfig` - 图形化配置

✅ **检查点**: 能看到帮助信息。

---

## 3. 编译第一个示例

### 3.1 加载示例配置

```bash
# 加载 sample_default 配置
python3 build.py config sample_default
```

你应该看到：
```
Loading config: sample_default
Configuration loaded: sample_default
Run 'python3 build.py build' to compile
```

### 3.2 编译项目

```bash
# 编译（使用所有 CPU 核心）
python3 build.py build
```

编译过程大约需要 **10-30 秒**。

你应该看到：
```
Building EMS SDK...
Configuring CMake...
Compiling...
[100%] Built target app_demo
Build successful!
Output directory: /path/to/EMS/_build
```

### 3.3 运行示例

```bash
# 运行示例程序
./_build/bin/app_demo
```

你应该看到：
```
=================================
  Sample Product Demo Application
=================================

=== EMS Version Information ===
Version:      1.0.0
...

Demo application running...
This is a sample application for demonstration purposes.
```

🎉 **恭喜！** 你已经成功编译并运行了第一个 EMS 应用！

✅ **检查点**: 程序正常运行，输出版本信息。

---

## 4. 理解项目结构

### 4.1 核心模块 (core/)

```
core/
├── osal/    # 操作系统抽象层 (线程、互斥锁、文件等)
├── hal/     # 硬件抽象层 (CAN、UART、I2C、SPI、GPIO)
├── pcl/     # 外设配置层 (硬件配置管理)
├── pdl/     # 外设驱动层 (卫星通信、BMC、MCU)
└── acl/     # 应用配置层 (业务逻辑配置)
```

**依赖关系**: acl → pdl → pcl → hal → osal

### 4.2 产品目录 (products/)

```
products/
├── sample/              # 示例产品
│   └── apps/
│       └── app_demo/    # 示例应用
└── ccm/                 # CCM 产品（卫星载荷管理）
    ├── apps/            # 5 个应用
    └── libs/            # 产品库
```

### 4.3 配置文件 (configs/)

```
configs/
├── sample_default_defconfig      # 示例配置
├── ccm_minimal_defconfig         # CCM 最小配置
├── ccm_h200_100p_v1_defconfig   # CCM 基础型号
└── ...
```

### 4.4 构建输出 (_build/)

```
_build/
├── bin/         # 可执行文件
├── lib/         # 库文件
└── config/      # 生成的配置文件
```

✅ **检查点**: 理解了项目的基本结构。

---

## 5. 修改示例代码

### 5.1 查看示例代码

```bash
# 查看示例应用源码
cat products/sample/apps/app_demo/src/main.c
```

代码很简单：
```c
#include <stdio.h>
#include "osal.h"

int main(int argc, char *argv[])
{
    printf("=================================\n");
    printf("  Sample Product Demo Application\n");
    printf("=================================\n\n");

    // 打印版本信息
    print_version_info();

    printf("\nDemo application running...\n");
    return 0;
}
```

### 5.2 修改代码

让我们添加一些功能：

```bash
# 编辑文件
nano products/sample/apps/app_demo/src/main.c
```

修改为：
```c
#include <stdio.h>
#include "osal.h"

int main(int argc, char *argv[])
{
    printf("=================================\n");
    printf("  My First EMS Application\n");
    printf("=================================\n\n");

    // 打印版本信息
    print_version_info();

    // 添加你的代码
    printf("\n🎉 Hello, EMS SDK!\n");
    printf("This is my first modification.\n");

    return 0;
}
```

### 5.3 重新编译

```bash
# 重新编译（只编译修改的文件，很快）
python3 build.py build

# 运行
./_build/bin/app_demo
```

你应该看到你的修改：
```
🎉 Hello, EMS SDK!
This is my first modification.
```

✅ **检查点**: 成功修改并运行了代码。

---

## 6. 配置功能模块

### 6.1 查看当前配置

```bash
# 查看当前启用的模块
grep "^CONFIG_" _build/config/global_config.cmake | grep "=y"
```

你会看到：
```
CONFIG_OSAL=y
CONFIG_HAL=y
CONFIG_PLATFORM_LINUX=y
...
```

### 6.2 图形化配置

```bash
# 打开配置界面
python3 build.py menuconfig
```

**配置界面操作**:
- `↑↓` - 移动光标
- `Space` - 选择/取消
- `Enter` - 进入子菜单
- `Esc Esc` - 返回上级
- `?` - 查看帮助
- `/` - 搜索

### 6.3 启用更多功能

在 menuconfig 中：

1. 进入 `Core Components` → `OSAL`
2. 启用 `IPC (Inter-Process Communication)`
3. 启用 `Thread Support`
4. 保存并退出

```bash
# 重新编译
python3 build.py build
```

现在你的应用可以使用线程和 IPC 功能了！

✅ **检查点**: 能够使用 menuconfig 配置功能。

---

## 7. 添加新应用

### 7.1 创建应用目录

```bash
# 创建新应用目录
mkdir -p products/sample/apps/my_app/src
```

### 7.2 编写应用代码

创建 `products/sample/apps/my_app/src/main.c`:

```c
#include <stdio.h>
#include "osal.h"

int main(int argc, char *argv[])
{
    printf("=================================\n");
    printf("  My Custom Application\n");
    printf("=================================\n\n");

    // 使用 OSAL API
    printf("OSAL initialized successfully!\n");

    return 0;
}
```

### 7.3 创建 CMakeLists.txt

创建 `products/sample/apps/my_app/CMakeLists.txt`:

```cmake
# 收集源文件
file(GLOB APP_SRCS "src/*.c")

# 创建可执行文件
add_executable(my_app ${APP_SRCS})

# 链接 OSAL 库
target_link_libraries(my_app
    PRIVATE
        osal
)
```

### 7.4 注册应用

编辑 `products/sample/CMakeLists.txt`，添加：

```cmake
# 在文件末尾添加
add_subdirectory(apps/my_app)
```

### 7.5 编译运行

```bash
# 重新配置和编译
python3 build.py distclean
python3 build.py config sample_default
python3 build.py build

# 运行你的应用
./_build/bin/my_app
```

🎉 **恭喜！** 你已经创建了自己的应用！

✅ **检查点**: 新应用能够编译和运行。

---

## 8. 下一步

### 8.1 学习核心模块

- **OSAL**: 阅读 `core/osal/README.md`，学习线程、互斥锁、信号量
- **HAL**: 阅读 `core/hal/README.md`，学习硬件驱动接口
- **PDL**: 阅读 `core/pdl/README.md`，学习外设驱动

### 8.2 查看更多示例

```bash
# 查看 CCM 产品的应用
ls products/ccm/apps/

# 查看测试代码
ls tests/unit/osal/
```

### 8.3 阅读进阶文档

- [开发者指南](DEVELOPER_GUIDE.md) - 如何添加新模块
- [架构概述](ARCHITECTURE.md) - 深入理解系统架构
- [API 参考](../core/*/README.md) - 各模块 API 文档

### 8.4 尝试其他配置

```bash
# 尝试 CCM 产品
python3 build.py config ccm_minimal
python3 build.py build

# 查看生成的应用
ls _build/bin/
```

---

## 🎓 学习检查清单

完成以下任务，确保你已经掌握基础知识：

- [ ] 成功安装所有依赖
- [ ] 编译并运行 sample_default 配置
- [ ] 修改示例代码并重新编译
- [ ] 使用 menuconfig 配置功能
- [ ] 创建并运行自己的应用
- [ ] 理解项目的目录结构
- [ ] 知道如何查看文档和示例

---

## 🐛 遇到问题？

### 编译失败

```bash
# 清理并重试
python3 build.py distclean
python3 build.py config sample_default
python3 build.py build
```

### 找不到命令

```bash
# 检查依赖是否安装
which gcc cmake python3
```

### 更多问题

查看 [故障排除指南](TROUBLESHOOTING.md)

---

## 💡 小贴士

1. **增量编译**: 修改代码后直接运行 `python3 build.py build`，只会编译修改的文件
2. **并行编译**: 构建脚本自动使用所有 CPU 核心
3. **配置缓存**: 配置会被缓存，不需要每次都重新配置
4. **查看日志**: 编译日志保存在 `_build/` 目录

---

## 🎉 恭喜！

你已经完成了 EMS SDK 的新手入门教程！

现在你可以：
- ✅ 编译和运行 EMS 应用
- ✅ 修改现有代码
- ✅ 配置功能模块
- ✅ 创建新应用

**下一步**: 阅读 [开发者指南](DEVELOPER_GUIDE.md)，学习如何添加新的驱动和模块。

---

**反馈**: 如果你在学习过程中遇到任何问题，欢迎提交 Issue！

**贡献**: 如果你发现文档有误或可以改进，欢迎提交 PR！
