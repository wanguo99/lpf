# Pure CMake Workflow Guide

ES-Middleware 支持完全无 Python 依赖的纯 CMake 构建工作流。

## 特性

✅ **零 Python 依赖** - 仅需要 CMake、Make 和 C 编译器  
✅ **原生 Kconfig 工具** - 使用 Buildroot 原生 C 实现的 conf/mconf/nconf  
✅ **完整功能** - 支持所有配置、构建、测试功能  
✅ **build.py 可选** - Python 脚本仅作为便捷工具，非必需  

## 系统要求

### 必需
- CMake 3.16+
- C/C++ 编译器（GCC/Clang）
- Make
- ncurses 开发库（用于 menuconfig）

### 安装依赖（Ubuntu/Debian）
```bash
sudo apt install build-essential cmake libncurses-dev
```

## 快速开始

### 1. 配置项目

#### 方式 A：加载预定义配置
```bash
# 复制 defconfig 到 .config
cp configs/tests_x86_minimal_defconfig .config

# 或者其他配置
cp configs/ccm_h200_100p_am625_debug_defconfig .config
```

#### 方式 B：使用 menuconfig
```bash
# 首次配置需要先运行 CMake 以构建 kconfig 工具
mkdir -p build && cd build
cmake ..

# 打开配置界面
make menuconfig

# 保存后退出，返回源码目录
cd ..
```

### 2. 构建项目

```bash
# 创建构建目录
mkdir -p build && cd build

# 配置 CMake
cmake ..

# 编译（使用所有 CPU 核心）
make -j$(nproc)

# 或单线程编译（便于调试）
make
```

### 3. 运行可执行文件

```bash
# 运行测试程序
./bin/es-middleware-test --help
./bin/es-middleware-test --all

# 运行其他应用（如果构建了 CCM 产品）
./bin/ccm_collector
```

## 配置管理

### 列出可用配置
```bash
ls configs/*_defconfig
```

输出示例：
```
configs/ccm_h200_100p_am625_debug_defconfig
configs/tests_x86_minimal_defconfig
configs/tests_x86_full_defconfig
```

### 交互式配置（menuconfig）
```bash
cd build
make menuconfig          # ncurses 界面（推荐）
# 或
make nconfig            # 替代 ncurses 界面

# 配置完成后重新运行 cmake
cmake ..
make -j$(nproc)
```

### 保存自定义配置
```bash
cd build
make savedefconfig      # 保存为 defconfig 格式

# 复制到 configs 目录
cp defconfig ../configs/my_custom_defconfig
```

### 更新配置文件
```bash
cd build
make oldconfig         # 使用默认值更新新增的 Kconfig 选项
```

## 清理构建

### 清理编译产物（保留配置）
```bash
cd build
make clean
```

### 完全清理（包括 CMake 缓存）
```bash
rm -rf build
```

### 清理配置文件
```bash
rm .config
```

## 完整工作流示例

### 示例 1：构建最小测试配置
```bash
# 1. 加载配置
cp configs/tests_x86_minimal_defconfig .config

# 2. 构建
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 3. 运行测试
./bin/es-middleware-test --all
```

### 示例 2：自定义配置并构建
```bash
# 1. 初始配置
mkdir -p build && cd build
cmake ..

# 2. 打开配置界面
make menuconfig
# 在界面中选择需要的功能，保存退出

# 3. 重新配置并构建
cmake ..
make -j$(nproc)

# 4. 保存自定义配置
make savedefconfig
cp defconfig ../configs/my_project_defconfig
```

### 示例 3：从零开始构建 CCM 产品
```bash
# 1. 加载 CCM 配置
cp configs/ccm_h200_100p_am625_debug_defconfig .config

# 2. 构建
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# 3. 查看生成的可执行文件
ls -lh bin/
```

## 可用的 Make 目标

在 `build/` 目录下：

| 目标 | 功能 |
|------|------|
| `make` | 构建所有目标 |
| `make -j$(nproc)` | 并行构建（推荐） |
| `make clean` | 清理编译产物 |
| `make menuconfig` | 打开配置界面（ncurses） |
| `make nconfig` | 打开替代配置界面 |
| `make savedefconfig` | 保存最小化配置 |
| `make oldconfig` | 更新配置文件 |
| `make list_defconfigs` | 列出可用的 defconfig |

## 与 build.py 的对比

| 操作 | 纯 CMake | build.py（可选） |
|------|----------|------------------|
| 加载配置 | `cp configs/<name>_defconfig .config` | `python3 build.py config <name>` |
| 配置界面 | `make menuconfig` | `python3 build.py menuconfig` |
| 构建 | `cmake .. && make -j$(nproc)` | `python3 build.py build` |
| 清理 | `rm -rf build` | `python3 build.py distclean` |
| 保存配置 | `make savedefconfig` | `python3 build.py savedefconfig` |

**build.py 的优势**：
- 一键式操作（自动处理目录切换）
- 配置验证和统计信息
- 更友好的错误提示

**纯 CMake 的优势**：
- 零 Python 依赖
- 标准 CMake 工作流
- 更好的 IDE 集成（CLion、VSCode CMake 插件）
- 更灵活的构建选项

## 常见问题

### Q: 找不到 ncurses 库
```bash
sudo apt install libncurses-dev
```

### Q: menuconfig 提示工具未构建
```bash
# 先运行一次 cmake 构建 kconfig 工具
cd build
cmake ..
make menuconfig
```

### Q: 修改 .config 后构建失败
```bash
# 重新运行 cmake 以重新加载配置
cd build
cmake ..
make -j$(nproc)
```

### Q: 如何查看当前配置
```bash
cat .config
# 或在 menuconfig 中查看
```

### Q: 如何切换配置
```bash
# 方法 1：直接复制
cp configs/another_defconfig .config
cd build && cmake ..

# 方法 2：使用 menuconfig
cd build
make menuconfig
# 选择 Load -> 加载其他配置文件
```

## 生成的文件

### 配置文件
- `.config` - 当前配置文件（Makefile 格式）
- `build/config/autoconf.h` - C 头文件（#define 宏）
- `build/config/kconfig.cmake` - CMake 变量文件
- `build/config/global_build_info_*.h` - 构建信息头文件

### 构建产物
- `build/bin/` - 可执行文件
- `build/lib/` - 静态库和共享库

### Kconfig 工具
- `build/kconfig-tools-build/conf` - 命令行配置工具
- `build/kconfig-tools-build/mconf` - menuconfig 工具
- `build/kconfig-tools-build/nconf` - nconfig 工具

## 高级用法

### 指定构建类型
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### 指定编译器
```bash
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ..
```

### 交叉编译
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm64.cmake ..
```

### 详细输出
```bash
make VERBOSE=1
```

## 总结

ES-Middleware 的纯 CMake 工作流提供了完整的无 Python 依赖构建体验，适合：

- 嵌入式 Linux 开发环境（可能缺少 Python）
- CI/CD 环境（最小化依赖）
- 使用 IDE 的开发者（CLion、VSCode）
- 喜欢标准工具链的开发者

同时，build.py 脚本作为可选便捷工具保留，提供更友好的用户体验。

---

**推荐工作流**：
- 开发环境：使用 build.py（更方便）
- CI/CD 环境：使用纯 CMake（更标准）
- 生产构建：两者皆可
