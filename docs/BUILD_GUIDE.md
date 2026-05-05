# EMS 构建指南

本文档提供 EMS 框架的完整构建说明，包括本地编译和交叉编译。

## 快速开始

### 使用 CMake Presets（推荐）

CMake 3.19+ 支持 presets 功能，提供预定义的构建配置。

```bash
# 查看可用配置
cmake --list-presets

# 本地编译
cmake --preset release && cmake --build build/release
cmake --preset debug && cmake --build build/debug

# 交叉编译
cmake --preset arm32 && cmake --build build/arm32
cmake --preset arm64 && cmake --build build/arm64
cmake --preset riscv64 && cmake --build build/riscv64
```

### 使用 build.sh（向后兼容）

```bash
./build.sh              # Release 模式
./build.sh -d           # Debug 模式
./build.sh -c           # 清理
./build.sh -a arm32     # ARM32 交叉编译
```

## 交叉编译

### 支持的架构

| 架构 | Preset | 工具链 | 状态 |
|------|--------|--------|------|
| x86_64 | native | gcc | ✅ 完全支持 |
| ARM32 | arm32 | arm-linux-gnueabihf-gcc | ✅ 完全支持 |
| ARM64 | arm64 | aarch64-linux-gnu-gcc | ✅ 完全支持 |
| RISC-V 64 | riscv64 | riscv64-linux-gnu-gcc | ✅ 完全支持 |

### 安装工具链

**Ubuntu/Debian:**
```bash
sudo apt-get install gcc-arm-linux-gnueabihf      # ARM32
sudo apt-get install gcc-aarch64-linux-gnu        # ARM64
sudo apt-get install gcc-riscv64-linux-gnu        # RISC-V 64
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-arm-linux-gnu                # ARM32
sudo dnf install gcc-aarch64-linux-gnu            # ARM64
sudo dnf install gcc-riscv64-linux-gnu            # RISC-V 64
```

### 架构特定编译选项

- **ARM32**: `-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard`
- **ARM64**: `-march=armv8-a`
- **RISC-V 64**: `-march=rv64imafdc -mabi=lp64d`

## 构建选项

```bash
cmake --preset release \
  -DBUILD_TESTING=OFF \
  -DBUILD_SHARED_LIBS=OFF
```

**可用选项:**
- `BUILD_TESTING`: 是否构建测试（默认 ON）
- `BUILD_SHARED_LIBS`: 是否构建动态库（默认 ON）
- `CMAKE_BUILD_TYPE`: 构建类型（Debug/Release）

## 输出目录

```
build/
├── release/          # Release 构建
│   ├── bin/          # 可执行文件
│   │   ├── sample_app
│   │   └── ems-test
│   └── lib/          # 库文件
│       ├── libosal.so
│       ├── libhal.so
│       ├── libpcl.so
│       └── libpdl.so
├── debug/            # Debug 构建
├── arm32/            # ARM32 交叉编译
├── arm64/            # ARM64 交叉编译
└── riscv64/          # RISC-V 64 交叉编译
```

## 运行程序

```bash
# 运行示例应用
./output/target/bin/sample_app

# 运行测试
./output/target/bin/ems-test -i     # 交互式
./output/target/bin/ems-test -a     # 运行所有测试
./output/target/bin/osal-test -a    # 仅运行OSAL层测试
```

## 构建系统架构

### 模块依赖层次

```
Apps → PDL → HAL → OSAL
         ↓
        PCL
```

### 接口库设计

每个模块提供两个库：
- **接口库** (`<module>_public_api`) - 仅头文件，用于编译时依赖
- **实现库** (`<module>`) - 静态/动态库，用于链接

**示例:**
```cmake
# 链接 OSAL
target_link_libraries(my_module 
    PUBLIC ems::osal_public_api    # 获取头文件
    PRIVATE ems::osal)             # 链接实现
```

## IDE 集成

### VS Code
安装 CMake Tools 插件，自动识别 `CMakePresets.json`。

### CLion
CLion 2021.1+ 原生支持 CMake Presets。

### Visual Studio
Visual Studio 2019 16.10+ 支持 CMake Presets。

## 常见问题

**Q: CMake 版本过低？**
```
CMake Error: CMake 3.19 or higher is required
```
升级 CMake 或使用标准 CMake 命令（不使用 presets）。

**Q: 找不到交叉编译工具链？**
安装对应的工具链（见上方安装说明）。

**Q: macOS 上编译？**
macOS 使用打桩实现的 HAL 层，仅用于编译验证，不支持实际硬件操作。

## 参考文档

- [架构设计](ARCHITECTURE.md)
- [编码规范](CODING_STANDARDS.md)
- [Buildroot 集成](buildroot/README.md)
