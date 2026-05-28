# EMS SDK 故障排除指南

本文档提供常见问题的解决方案和调试技巧。

---

## 📋 目录

1. [编译问题](#1-编译问题)
2. [配置问题](#2-配置问题)
3. [运行时问题](#3-运行时问题)
4. [环境问题](#4-环境问题)
5. [调试技巧](#5-调试技巧)

---

## 1. 编译问题

### 1.1 找不到 python3 命令

**错误信息**:
```
bash: python3: command not found
```

**解决方案**:
```bash
# Ubuntu/Debian
sudo apt-get install python3

# macOS
brew install python3

# 验证安装
python3 --version
```

---

### 1.2 CMake 版本过低

**错误信息**:
```
CMake 3.16 or higher is required. You are running version 3.10
```

**解决方案**:
```bash
# Ubuntu 20.04+
sudo apt-get update
sudo apt-get install cmake

# 或从官方下载最新版本
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-linux-x86_64.sh
sudo sh cmake-3.27.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

---

### 1.3 编译失败：找不到头文件

**错误信息**:
```
fatal error: osal.h: No such file or directory
```

**原因**: 配置未正确加载或构建目录损坏

**解决方案**:
```bash
# 完全清理并重新构建
python3 build.py distclean
python3 build.py config sample_default
python3 build.py build
```

---

### 1.4 链接错误：undefined reference

**错误信息**:
```
undefined reference to `OSAL_ThreadCreate'
```

**原因**: 缺少必要的库或模块未启用

**解决方案**:
```bash
# 1. 检查配置
grep CONFIG_OSAL _build/config/global_config.cmake

# 2. 确保 OSAL 已启用
python3 build.py menuconfig
# 进入 Core Components → OSAL，确保已启用

# 3. 重新编译
python3 build.py build
```

---

### 1.5 编译速度慢

**问题**: 编译时间过长

**解决方案**:
```bash
# 使用并行编译（默认已启用）
python3 build.py build

# 如果还是慢，检查 CPU 核心数
nproc

# 手动指定并行数
cmake --build _build -j8
```

---

### 1.6 No HAL driver source files selected

**错误信息**:
```
CMake Error: No HAL driver source files selected. Please check your Kconfig configuration.
```

**原因**: HAL 平台未配置或驱动未启用

**解决方案**:
```bash
# 方法 1: 使用预定义配置
python3 build.py config sample_default

# 方法 2: 手动配置
python3 build.py menuconfig
# 进入 Core Components → HAL
# 启用 Platform Configuration → Linux platform
# 启用至少一个驱动（如 CAN bus support）

# 重新编译
python3 build.py build
```

---

## 2. 配置问题

### 2.1 menuconfig 无法运行

**错误信息**:
```
ModuleNotFoundError: No module named 'menuconfig'
```

**解决方案**:
```bash
# 安装 ncurses 开发库
sudo apt-get install libncurses-dev

# 如果还是失败，检查 Python 路径
which python3
python3 -c "import sys; print(sys.path)"
```

---

### 2.2 配置不生效

**问题**: 修改配置后编译结果没有变化

**解决方案**:
```bash
# 1. 清理构建缓存
python3 build.py distclean

# 2. 重新加载配置
python3 build.py config <your_config>

# 3. 验证配置
cat _build/config/global_config.cmake | grep CONFIG_

# 4. 重新编译
python3 build.py build
```

---

### 2.3 找不到配置文件

**错误信息**:
```
Error: Config file not found: /path/to/EMS/configs/xxx_defconfig
```

**解决方案**:
```bash
# 查看可用配置
ls configs/*_defconfig

# 使用正确的配置名（不带 _defconfig 后缀）
python3 build.py config sample_default
```

---

### 2.4 配置冲突

**问题**: 某些配置选项互相冲突

**解决方案**:
```bash
# 使用预定义配置作为起点
python3 build.py config sample_default

# 然后使用 menuconfig 修改
python3 build.py menuconfig

# 保存配置
# 在 menuconfig 中选择 Save，然后 Exit
```

---

## 3. 运行时问题

### 3.1 找不到可执行文件

**问题**: 编译成功但找不到可执行文件

**解决方案**:
```bash
# 查看构建输出
ls _build/bin/

# 如果目录为空，检查配置
python3 build.py config sample_default
python3 build.py build

# 查看编译日志
python3 build.py build 2>&1 | tee build.log
```

---

### 3.2 找不到共享库

**错误信息**:
```
error while loading shared libraries: libosal.so: cannot open shared object file
```

**解决方案**:
```bash
# 方法 1: 设置 LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PWD/_build/lib:$LD_LIBRARY_PATH
./_build/bin/app_demo

# 方法 2: 使用静态库配置
python3 build.py menuconfig
# 进入各模块，启用 "Build static library"
python3 build.py build
```

---

### 3.3 段错误 (Segmentation fault)

**问题**: 程序运行时崩溃

**调试步骤**:
```bash
# 1. 使用 Debug 配置重新编译
python3 build.py menuconfig
# 设置 Build type → Debug

python3 build.py build

# 2. 使用 gdb 调试
gdb ./_build/bin/app_demo
(gdb) run
(gdb) bt  # 查看调用栈

# 3. 使用 valgrind 检查内存问题
valgrind --leak-check=full ./_build/bin/app_demo
```

---

### 3.4 权限错误

**错误信息**:
```
Permission denied
```

**解决方案**:
```bash
# 添加执行权限
chmod +x ./_build/bin/app_demo

# 或者使用 sudo（不推荐）
sudo ./_build/bin/app_demo
```

---

## 4. 环境问题

### 4.1 交叉编译工具链未找到

**错误信息**:
```
aarch64-linux-gnu-gcc: command not found
```

**解决方案**:
```bash
# Ubuntu/Debian
sudo apt-get install gcc-aarch64-linux-gnu

# 验证安装
aarch64-linux-gnu-gcc --version

# 配置交叉编译
python3 build.py menuconfig
# 设置 Toolchain path 和 Toolchain prefix
```

---

### 4.2 磁盘空间不足

**错误信息**:
```
No space left on device
```

**解决方案**:
```bash
# 清理构建目录
python3 build.py distclean

# 清理 Git 缓存
git gc --aggressive

# 检查磁盘空间
df -h
```

---

### 4.3 Git 子模块问题

**问题**: 某些依赖未正确下载

**解决方案**:
```bash
# 初始化并更新子模块
git submodule update --init --recursive

# 如果还有问题，重新克隆
cd ..
rm -rf EMS
git clone --recursive https://github.com/wanguo99/EMS.git
```

---

## 5. 调试技巧

### 5.1 查看详细编译日志

```bash
# 保存完整日志
python3 build.py build 2>&1 | tee build.log

# 查看 CMake 配置
cat _build/CMakeCache.txt

# 查看生成的配置
cat _build/config/global_config.cmake
```

---

### 5.2 检查配置状态

```bash
# 查看当前配置
grep "^CONFIG_" _build/config/global_config.cmake | grep "=y"

# 查看特定模块配置
grep "CONFIG_OSAL" _build/config/global_config.cmake
grep "CONFIG_HAL" _build/config/global_config.cmake
```

---

### 5.3 增量编译调试

```bash
# 只编译特定目标
cmake --build _build --target osal

# 查看编译命令
cmake --build _build --verbose

# 强制重新编译
cmake --build _build --clean-first
```

---

### 5.4 使用调试宏

在代码中添加调试输出：

```c
#include "util/osal_log.h"

// 使用 OSAL 日志
OSAL_LOG_DEBUG("Debug message: %d", value);
OSAL_LOG_INFO("Info message");
OSAL_LOG_ERROR("Error: %s", error_msg);
```

启用调试日志：
```bash
python3 build.py menuconfig
# 进入 Core Components → OSAL → Debug logging
python3 build.py build
```

---

### 5.5 性能分析

```bash
# 使用 time 测量编译时间
time python3 build.py build

# 使用 perf 分析运行时性能
perf record ./_build/bin/app_demo
perf report

# 使用 gprof 分析
# 编译时添加 -pg 选项
./_build/bin/app_demo
gprof ./_build/bin/app_demo gmon.out
```

---

## 6. 常见问题 FAQ

### Q1: 如何切换不同的配置？

```bash
python3 build.py distclean
python3 build.py config <new_config>
python3 build.py build
```

### Q2: 如何查看所有可用配置？

```bash
ls configs/*_defconfig
```

### Q3: 如何保存自定义配置？

```bash
python3 build.py menuconfig
# 修改配置后保存
# 配置保存在 _build/config/ 目录
```

### Q4: 如何添加新的源文件？

修改对应的 `CMakeLists.txt`，然后重新编译：
```bash
python3 build.py build
```

### Q5: 如何查看编译产物？

```bash
# 可执行文件
ls _build/bin/

# 库文件
ls _build/lib/

# 查看文件大小
du -sh _build/
```

### Q6: 如何清理特定模块？

```bash
# 删除特定模块的构建产物
rm -rf _build/products/sample/osal/

# 重新编译
python3 build.py build
```

---

## 7. 获取帮助

### 7.1 查看文档

- [新手入门](GETTING_STARTED.md)
- [开发者指南](DEVELOPER_GUIDE.md)
- [架构概述](ARCHITECTURE.md)
- [构建指南](CMAKE_BUILD_GUIDE.md)

### 7.2 搜索 Issues

访问 [GitHub Issues](https://github.com/wanguo99/EMS/issues) 搜索类似问题。

### 7.3 提交 Issue

如果问题未解决，请提交 Issue 并包含：

1. **问题描述**: 详细描述问题
2. **环境信息**: 操作系统、工具版本
3. **重现步骤**: 如何重现问题
4. **错误日志**: 完整的错误信息
5. **配置信息**: 使用的配置文件

示例：
```markdown
## 问题描述
编译时出现链接错误

## 环境信息
- OS: Ubuntu 22.04
- GCC: 11.3.0
- CMake: 3.22.1

## 重现步骤
1. python3 build.py config sample_default
2. python3 build.py build

## 错误日志
```
undefined reference to `OSAL_ThreadCreate'
```

## 配置信息
使用 sample_default 配置
```

---

## 8. 调试检查清单

遇到问题时，按以下顺序检查：

- [ ] 检查依赖是否安装完整
- [ ] 检查 Python、CMake、GCC 版本
- [ ] 清理构建目录 (`distclean`)
- [ ] 重新加载配置
- [ ] 查看详细编译日志
- [ ] 检查配置是否正确生效
- [ ] 搜索相关 Issues
- [ ] 查看相关文档
- [ ] 提交 Issue 寻求帮助

---

## 9. 紧急救援

如果所有方法都失败了：

```bash
# 完全重置
cd ..
rm -rf EMS
git clone https://github.com/wanguo99/EMS.git
cd EMS

# 使用最简单的配置
python3 build.py config sample_default
python3 build.py build

# 如果还是失败，提交 Issue
```

---

**提示**: 大多数问题都可以通过 `distclean` + 重新配置解决。

**反馈**: 如果你发现新的问题或解决方案，欢迎更新本文档！
