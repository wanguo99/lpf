# 快速编译指南

EMS 框架支持标准 CMake 工作流，无需依赖自定义脚本即可轻松编译。

## 方式一：使用 CMake Presets（推荐）

CMake 3.19+ 支持 presets 功能，提供预定义的构建配置。

### 查看可用配置

```bash
cmake --list-presets
```

输出：
```
Available configure presets:
  "debug"   - Debug (Native)
  "release" - Release (Native)
  "arm32"   - ARM32 Cross-Compile
  "arm64"   - ARM64 Cross-Compile
  "riscv64" - RISC-V 64 Cross-Compile
```

### 本地编译

**Release 版本（推荐）：**
```bash
cmake --preset release
cmake --build build/release
```

**Debug 版本：**
```bash
cmake --preset debug
cmake --build build/debug
```

### 交叉编译

**ARM32 (ARMv7-A)：**
```bash
cmake --preset arm32
cmake --build build/arm32
```

**ARM64 (ARMv8-A)：**
```bash
cmake --preset arm64
cmake --build build/arm64
```

**RISC-V 64：**
```bash
cmake --preset riscv64
cmake --build build/riscv64
```

### 清理构建

```bash
rm -rf build
```

## 方式二：标准 CMake 命令

如果不使用 presets，可以使用标准 CMake 命令。

### 本地编译

```bash
# 配置
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build

# 或者使用 make
cd build && make -j$(nproc)
```

### 交叉编译

```bash
# ARM32
cmake -B build-arm32 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm32-linux-gnueabihf.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-arm32

# ARM64
cmake -B build-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-arm64

# RISC-V 64
cmake -B build-riscv64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-linux-gnu.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-riscv64
```

## 方式三：使用 build.sh（向后兼容）

保留了原有的 build.sh 脚本以保持向后兼容。

```bash
./build.sh              # Release 模式
./build.sh -d           # Debug 模式
./build.sh -c           # 清理
./build.sh -a arm32     # ARM32 交叉编译
./build.sh -a arm64     # ARM64 交叉编译
./build.sh -a riscv64   # RISC-V 64 交叉编译
```

## 构建选项

可以通过 `-D` 参数自定义构建选项：

```bash
cmake --preset release \
  -DBUILD_TESTING=OFF \
  -DBUILD_SHARED_LIBS=OFF
```

**可用选项：**
- `BUILD_TESTING`: 是否构建测试（默认 ON）
- `BUILD_SHARED_LIBS`: 是否构建动态库（默认 ON，OFF 为静态库）
- `CMAKE_BUILD_TYPE`: 构建类型（Debug/Release）

## 输出目录

使用 presets 时，输出目录结构：

```
build/
├── release/          # Release 构建
│   ├── bin/          # 可执行文件
│   │   ├── sample_app
│   │   └── unit-test
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
./build/release/bin/sample_app

# 运行测试（交互式）
./build/release/bin/unit-test -i

# 运行所有测试
./build/release/bin/unit-test -a
```

## 安装交叉编译工具链

**Ubuntu/Debian：**
```bash
sudo apt-get install gcc-arm-linux-gnueabihf      # ARM32
sudo apt-get install gcc-aarch64-linux-gnu        # ARM64
sudo apt-get install gcc-riscv64-linux-gnu        # RISC-V 64
```

**macOS (Homebrew)：**
```bash
brew install arm-linux-gnueabihf-binutils
brew install aarch64-elf-gcc
brew install riscv-gnu-toolchain
```

## 常见问题

### 1. CMake 版本过低

```
CMake Error: CMake 3.19 or higher is required
```

**解决方法：** 升级 CMake 或使用方式二（标准 CMake 命令）。

### 2. 找不到交叉编译工具链

```
CMake Error: Could not find toolchain file
```

**解决方法：** 安装对应的交叉编译工具链（见上方安装说明）。

### 3. macOS 上编译

macOS 使用打桩实现的 HAL 层，仅用于编译验证，不支持实际硬件操作。生成的库文件扩展名为 `.dylib`（macOS 动态库格式）。

## IDE 集成

### VS Code

安装 CMake Tools 插件后，会自动识别 `CMakePresets.json`，可以在状态栏选择配置。

### CLion

CLion 2021.1+ 原生支持 CMake Presets，打开项目后会自动加载配置。

### Visual Studio

Visual Studio 2019 16.10+ 支持 CMake Presets，打开文件夹后会自动识别。

## 更多信息

- [构建系统架构](BUILD_SYSTEM.md)
- [交叉编译指南](CROSS_COMPILE_GUIDE.md)
- [Buildroot 集成](buildroot/README.md)
