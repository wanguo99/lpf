# EMS 交叉编译支持

## 概述

EMS 框架**完全支持交叉编译**，可以在 x86_64 主机上编译出适用于不同目标平台的二进制文件。

## 支持的目标架构

| 架构 | 参数 | 工具链 | 编译选项 | 状态 |
|------|------|--------|----------|------|
| **本地架构** | `native` | 系统默认 | 自动检测 | ✅ 完全支持 |
| **x86_64** | `x86_64` | gcc | - | ✅ 完全支持 |
| **ARM32** | `arm32` | arm-linux-gnueabihf-gcc | `-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard` | ✅ 完全支持 |
| **ARM64** | `arm64` | aarch64-linux-gnu-gcc | `-march=armv8-a` | ✅ 完全支持 |
| **RISC-V 64** | `riscv64` | riscv64-linux-gnu-gcc | `-march=rv64imafdc -mabi=lp64d` | ✅ 完全支持 |

## 快速开始

### 1. 安装交叉编译工具链

#### Ubuntu/Debian

```bash
# ARM32 (ARMv7-A)
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# ARM64 (ARMv8-A)
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# RISC-V 64
sudo apt-get install gcc-riscv64-linux-gnu g++-riscv64-linux-gnu
```

#### Fedora/RHEL

```bash
# ARM32
sudo dnf install gcc-arm-linux-gnu

# ARM64
sudo dnf install gcc-aarch64-linux-gnu

# RISC-V 64
sudo dnf install gcc-riscv64-linux-gnu
```

#### macOS (Homebrew)

```bash
# ARM64 (Apple Silicon 原生支持)
# ARM32 和 RISC-V 需要使用 Docker 或虚拟机

# 推荐使用 Docker
docker pull ubuntu:22.04
docker run -it -v $(pwd):/workspace ubuntu:22.04
# 然后在容器内安装工具链
```

### 2. 交叉编译

```bash
# ARM32 (ARMv7-A)
./build.sh -a arm32

# ARM64 (ARMv8-A)
./build.sh -a arm64

# RISC-V 64
./build.sh -a riscv64

# Debug 模式
./build.sh -a arm64 -d

# 清理后重新编译
./build.sh -c && ./build.sh -a arm32
```

### 3. 验证编译结果

```bash
# 查看生成的二进制文件
ls -lh output/target/bin/
ls -lh output/target/lib/

# 检查二进制文件架构
file output/target/bin/sample_app
file output/target/lib/libosal.so

# ARM32 示例输出：
# sample_app: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV)

# ARM64 示例输出：
# sample_app: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV)

# RISC-V 64 示例输出：
# sample_app: ELF 64-bit LSB executable, UCB RISC-V, version 1 (SYSV)
```

## 交叉编译原理

### 构建流程

```
┌─────────────────┐
│  build.sh       │  1. 检测目标架构
│  -a arm64       │  2. 验证工具链
└────────┬────────┘
         │
         v
┌─────────────────┐
│  CMakeLists.txt │  3. 设置交叉编译器
│  PLATFORM=      │  4. 配置编译选项
│  arm64-linux    │  5. 设置系统信息
└────────┬────────┘
         │
         v
┌─────────────────┐
│  aarch64-linux- │  6. 编译源代码
│  gnu-gcc        │  7. 链接库文件
└────────┬────────┘
         │
         v
┌─────────────────┐
│  output/target/ │  8. 生成目标文件
│  ARM64 binaries │     (ARM64 ELF)
└─────────────────┘
```

### 关键配置

#### 1. 编译器设置

```cmake
# CMakeLists.txt
if(PLATFORM STREQUAL "arm64-linux")
    set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
endif()
```

#### 2. 架构特定选项

```cmake
# ARM32: ARMv7-A + NEON + 硬浮点
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard")

# ARM64: ARMv8-A
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a")

# RISC-V 64: RV64IMAFDC (完整指令集)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=rv64imafdc -mabi=lp64d")
```

#### 3. 库查找路径

```cmake
# 只在目标系统路径中查找库
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

## 类型系统保证

EMS 的类型系统经过优化，确保跨平台兼容性：

### 固定宽度类型

```c
int8_t, int16_t, int32_t, int64_t      // 所有平台大小一致
uint8_t, uint16_t, uint32_t, uint64_t  // 所有平台大小一致
```

### 平台相关类型

```c
// 自动适配目标平台
osal_size_t      // ARM32: 4字节, ARM64: 8字节
osal_uintptr_t   // ARM32: 4字节, ARM64: 8字节
```

### 编译时验证

```c
// 自动验证类型大小
OSAL_STATIC_ASSERT(sizeof(int32_t) == 4, int32_must_be_4_bytes);
OSAL_STATIC_ASSERT(sizeof(osal_uintptr_t) == sizeof(void*), uintptr_must_match_pointer);
```

## 常见问题

### 1. 工具链未找到

**问题**：
```
[ERROR] 交叉编译工具链未找到: arm-linux-gnueabihf-gcc
```

**解决方案**：
```bash
# 安装对应的工具链
sudo apt-get install gcc-arm-linux-gnueabihf

# 验证安装
arm-linux-gnueabihf-gcc --version
```

### 2. 链接错误

**问题**：
```
undefined reference to `pthread_create'
```

**解决方案**：
- 确保安装了完整的工具链（包括 libc 和 pthread）
- 检查 CMakeLists.txt 中的 `target_link_libraries`

### 3. 运行时错误

**问题**：
```
bash: ./sample_app: cannot execute binary file: Exec format error
```

**原因**：
- 在 x86_64 主机上运行 ARM 二进制文件

**解决方案**：
```bash
# 方案1: 在目标硬件上运行
scp output/target/bin/sample_app user@arm-device:/tmp/
ssh user@arm-device /tmp/sample_app

# 方案2: 使用 QEMU 模拟器
sudo apt-get install qemu-user-static
qemu-aarch64-static output/target/bin/sample_app

# 方案3: 使用 Docker + QEMU
docker run --rm -v $(pwd):/workspace arm64v8/ubuntu /workspace/output/target/bin/sample_app
```

### 4. 头文件路径问题

**问题**：
```
fatal error: linux/can.h: No such file or directory
```

**解决方案**：
- 安装目标架构的内核头文件
```bash
# ARM64
sudo apt-get install linux-headers-arm64-cross

# 或者使用 sysroot
./build.sh -a arm64 -DCMAKE_SYSROOT=/path/to/arm64-sysroot
```

## 高级用法

### 1. 自定义工具链

```bash
# 使用自定义编译器
cmake ../.. \
    -DPLATFORM=arm64-linux \
    -DCMAKE_C_COMPILER=/opt/custom-toolchain/bin/aarch64-linux-gnu-gcc
```

### 2. 指定 Sysroot

```bash
# 使用目标系统的根文件系统
cmake ../.. \
    -DPLATFORM=arm64-linux \
    -DCMAKE_SYSROOT=/path/to/arm64-rootfs
```

### 3. 静态链接

```bash
# 生成静态链接的可执行文件
cmake ../.. \
    -DPLATFORM=arm64-linux \
    -DBUILD_SHARED_LIBS=OFF
```

### 4. 优化级别

```bash
# Release 模式 (默认 -O3)
./build.sh -a arm64 -r

# Debug 模式 (-O0 -g)
./build.sh -a arm64 -d

# 自定义优化
cmake ../.. \
    -DPLATFORM=arm64-linux \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-O2 -flto"
```

## 性能优化

### ARM32 优化

```cmake
# 启用 NEON SIMD 指令
-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard

# 针对特定 CPU
-mcpu=cortex-a9  # Cortex-A9
-mcpu=cortex-a53 # Cortex-A53
```

### ARM64 优化

```cmake
# 基础 ARMv8-A
-march=armv8-a

# 针对特定 CPU
-mcpu=cortex-a53  # Cortex-A53
-mcpu=cortex-a72  # Cortex-A72
```

### RISC-V 优化

```cmake
# 完整指令集
-march=rv64imafdc -mabi=lp64d

# 仅基础指令集（更小的代码）
-march=rv64i -mabi=lp64
```

## CI/CD 集成

### GitHub Actions

```yaml
name: Cross-Compile

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        arch: [arm32, arm64, riscv64]
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install toolchain
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          gcc-arm-linux-gnueabihf \
          gcc-aarch64-linux-gnu \
          gcc-riscv64-linux-gnu
    
    - name: Build
      run: ./build.sh -a ${{ matrix.arch }}
    
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: ems-\${{ matrix.arch }}
        path: output/target/
```

### GitLab CI

```yaml
stages:
  - build

.cross-compile:
  stage: build
  image: ubuntu:22.04
  before_script:
    - apt-get update
    - apt-get install -y cmake make
  script:
    - ./build.sh -a ${ARCH}
  artifacts:
    paths:
      - output/target/

build:arm32:
  extends: .cross-compile
  variables:
    ARCH: arm32
  before_script:
    - apt-get install -y gcc-arm-linux-gnueabihf

build:arm64:
  extends: .cross-compile
  variables:
    ARCH: arm64
  before_script:
    - apt-get install -y gcc-aarch64-linux-gnu

build:riscv64:
  extends: .cross-compile
  variables:
    ARCH: riscv64
  before_script:
    - apt-get install -y gcc-riscv64-linux-gnu
```

## 总结

EMS 框架的交叉编译支持特性：

✅ **完整支持** ARM32/ARM64/RISC-V 64 交叉编译  
✅ **类型安全** 编译时验证跨平台类型一致性  
✅ **自动检测** 工具链和目标架构自动识别  
✅ **优化选项** 针对不同架构的性能优化  
✅ **易于使用** 一条命令完成交叉编译  
✅ **CI/CD 友好** 适合自动化构建流程  

**一套代码，多架构部署！**
