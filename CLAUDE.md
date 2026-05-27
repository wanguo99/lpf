# EMS 项目指南 - Claude AI 助手

本文档为 AI 助手提供 EMS 项目的完整上下文，帮助理解项目架构、构建系统和开发规范。

## ⚠️ 重要规则

### 禁止随意生成文档
- **严禁**在完成任务后自动生成总结文档、验证报告、README 等
- **严禁**创建 Markdown 格式的总结、清单、变更日志文件
- **只有**用户明确要求时才可以生成文档
- 完成任务后，**只需**简短口头汇报结果，不要创建文件

### 允许的文档操作
- ✅ 修改已存在的文档（如更新 CLAUDE.md）
- ✅ 用户明确要求创建的文档
- ✅ 代码注释和 commit message
- ❌ 自动生成的总结报告
- ❌ 自动生成的验证报告
- ❌ 自动生成的 TODO 列表文件

## 项目概述

EMS (Embedded Management System) 是一个采用 **Kconfig + CMake** 混合构建系统的嵌入式软件项目。

### 核心特点

- **配置系统**: Kconfig（menuconfig 图形化配置）
- **构建系统**: CMake 3.16+
- **架构模式**: Core/Products 分层架构
- **编程语言**: C 语言（C99 标准）
- **目标平台**: Linux 嵌入式系统（主要是 TI AM62x）

### 构建流程

```bash
# 1. 配置功能模块（Kconfig）
make menuconfig

# 2. 生成构建文件（CMake）
cmake -B build-cmake

# 3. 编译项目
cmake --build build-cmake -j$(nproc)
```

## 项目架构

### 目录结构

```
EMS/
├── core/                   # 核心模块（可复用组件）
│   ├── acl/               # 访问控制层
│   ├── hal/               # 硬件抽象层（CAN、UART、I2C、SPI、GPIO 等）
│   ├── osal/              # 操作系统抽象层（线程、互斥锁、信号量等）
│   ├── pcl/               # 协议控制层
│   └── pdl/               # 协议数据层
├── products/              # 产品应用（特定产品的实现）
│   └── ccm/              # CCM 产品
│       ├── apps/         # 应用程序（collector、logger、health、supervisor、comm）
│       ├── libs/         # 产品库（libccm）
│       └── h200_am625/   # 平台特定库
├── tests/                 # 测试代码
│   └── core/             # 核心模块测试
├── configs/               # Kconfig 配置文件（defconfig）
├── scripts/               # 构建脚本
│   └── kconfig/          # Kconfig 配置工具
├── cmake/                 # CMake 模块
│   └── Kconfig.cmake     # Kconfig 集成模块
├── include/               # 公共头文件目录
├── build-cmake/           # CMake 构建输出目录（不提交到 git）
│   ├── bin/              # 可执行文件
│   └── lib/              # 库文件
└── docs/                  # 项目文档
```

### 模块依赖关系

```
products/ccm/apps/*  →  h200_am625  →  core/acl  →  core/pdl  →  core/pcl  →  core/hal  →  core/osal
                     →  libccm      →  core/osal
```

**重要规则**:
- Products 依赖 Core，但 Core 不能依赖 Products
- 依赖关系通过 CMake 的 `target_link_libraries` 自动管理
- CMake 会自动处理传递依赖（transitive dependencies）

## 配置系统（Kconfig）

### Kconfig 配置流程

```bash
# 图形化配置界面
make menuconfig

# 加载预定义配置
make x86_64_full_defconfig
make arm64_full_defconfig

# 保存最小化配置
make savedefconfig
```

### 配置文件

- **`.config`**: 当前配置文件（由 menuconfig 生成）
- **`configs/*.defconfig`**: 预定义配置模板
- **`Kconfig`**: 配置选项定义文件

### 常用配置选项

```kconfig
# 核心模块开关
CONFIG_OSAL=y              # 操作系统抽象层
CONFIG_HAL=y               # 硬件抽象层
CONFIG_PCL=y               # 协议控制层
CONFIG_PDL=y               # 协议数据层
CONFIG_ACL=y               # 访问控制层

# 平台配置
CONFIG_ARCH_X86_64=y       # x86_64 架构
CONFIG_ARCH_ARM64=y        # ARM64 架构
CONFIG_OS_LINUX=y          # Linux 操作系统

# 功能裁剪
CONFIG_BUILD_TESTING=y     # 构建测试程序
CONFIG_BUILD_SHARED=y      # 构建共享库
```

### 添加新的配置选项

在相应模块的 `Kconfig` 文件中添加：

```kconfig
config MY_NEW_FEATURE
    bool "Enable my new feature"
    default y
    help
      This enables my new feature.
```

在 `CMakeLists.txt` 中使用：

```cmake
if(CONFIG_MY_NEW_FEATURE)
    add_subdirectory(my_feature)
endif()
```

## 构建系统（CMake）

### CMake 构建流程

```bash
# 1. 配置阶段（生成构建文件）
cmake -B build-cmake -S .

# 2. 编译阶段
cmake --build build-cmake -j$(nproc)

# 3. 安装阶段（可选）
cmake --install build-cmake --prefix /usr/local
```

### 构建选项

```bash
# 构建类型
cmake -B build-cmake -DCMAKE_BUILD_TYPE=Debug      # 调试版本（-O0 -g3）
cmake -B build-cmake -DCMAKE_BUILD_TYPE=Release    # 发布版本（-O2）

# 交叉编译
cmake -B build-cmake -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc

# 自定义安装路径
cmake -B build-cmake -DCMAKE_INSTALL_PREFIX=/opt/ems
```

### Kconfig 与 CMake 集成

CMake 通过 `cmake/Kconfig.cmake` 模块自动读取 `.config` 文件：

1. **加载配置**: `load_kconfig("${CMAKE_SOURCE_DIR}/.config")`
2. **设置变量**: 将 `CONFIG_XXX=y` 转换为 CMake 变量
3. **生成头文件**: 导出到 `autoconf.h` 供 C 代码使用

示例：
```cmake
# CMakeLists.txt
if(CONFIG_OSAL)
    add_subdirectory(core/osal)
endif()
```

### 库构建规范

每个库模块的 CMakeLists.txt 遵循统一模式：

```cmake
# 1. 收集源文件
file(GLOB LIB_SRCS "src/*.c")

# 2. 创建共享库
add_library(mylib SHARED ${LIB_SRCS})

# 3. 设置库版本（SONAME）
set_target_properties(mylib PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

# 4. 设置头文件路径
target_include_directories(mylib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 5. 链接依赖
target_link_libraries(mylib
    PUBLIC
        dependency1
        dependency2
)

# 6. 创建静态库（可选）
add_library(mylib_static STATIC ${LIB_SRCS})
set_target_properties(mylib_static PROPERTIES OUTPUT_NAME mylib)
```

### 应用程序构建规范

```cmake
# 1. 收集源文件
file(GLOB APP_SRCS "src/*.c")

# 2. 创建可执行文件
add_executable(myapp ${APP_SRCS})

# 3. 链接库（只需声明直接依赖）
target_link_libraries(myapp
    PRIVATE
        h200_am625
        ccm
)
```

**注意**: CMake 会自动处理传递依赖，应用程序只需链接直接使用的库。

## 编码规范

### C 语言规范

- **标准**: C99（支持 `//` 注释和现代 C 特性）
- **风格**: Linux 内核编码风格
- **缩进**: Tab（8 空格宽度）
- **命名**: 小写加下划线（snake_case）

### POSIX 特性宏

项目自动定义以下宏以启用 POSIX 扩展：

```c
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
```

这些宏确保可以使用 `clock_gettime`、`pthread_*` 等 POSIX 函数。

### CMakeLists.txt 规范

```cmake
# 1. 使用 file(GLOB) 收集源文件
file(GLOB SRCS "src/*.c")

# 2. 使用 target_* 命令而非全局命令
target_include_directories()  # ✅ 推荐
target_link_libraries()       # ✅ 推荐
include_directories()         # ❌ 避免使用

# 3. 使用生成器表达式处理不同场景
$<BUILD_INTERFACE:...>        # 构建时使用
$<INSTALL_INTERFACE:...>      # 安装后使用

# 4. 明确指定依赖的可见性
PUBLIC    # 依赖会传递给使用者
PRIVATE   # 依赖不会传递
INTERFACE # 仅传递，自己不使用
```

## 常见开发任务

### 添加新的库

1. 在 `core/mylib/Kconfig` 中添加配置选项：
```kconfig
config MYLIB
    bool "Enable MyLib"
    default y
    help
      My new library.
```

2. 在 `core/Kconfig` 中引用：
```kconfig
source "core/mylib/Kconfig"
```

3. 创建 `core/mylib/CMakeLists.txt`：
```cmake
file(GLOB MYLIB_SRCS "src/*.c")

add_library(mylib SHARED ${MYLIB_SRCS})

set_target_properties(mylib PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

target_include_directories(mylib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(mylib
    PUBLIC
        osal
)
```

4. 在 `core/CMakeLists.txt` 中添加：
```cmake
if(CONFIG_MYLIB)
    add_subdirectory(mylib)
endif()
```

5. 配置并编译：
```bash
make menuconfig  # 启用 MYLIB
cmake -B build-cmake
cmake --build build-cmake
```

### 添加新的应用程序

1. 创建 `products/ccm/apps/myapp/CMakeLists.txt`：
```cmake
file(GLOB MYAPP_SRCS "src/*.c")

add_executable(myapp ${MYAPP_SRCS})

target_link_libraries(myapp
    PRIVATE
        h200_am625
        ccm
)
```

2. 在父目录 `CMakeLists.txt` 中添加：
```cmake
add_subdirectory(apps/myapp)
```

3. 编译：
```bash
cmake --build build-cmake
```

### 功能裁剪

通过 Kconfig 配置实现功能裁剪：

```bash
# 1. 进入配置界面
make menuconfig

# 2. 关闭不需要的模块
#    [ ] HAL (Hardware Abstraction Layer)
#    [ ] PCL (Protocol Control Layer)

# 3. 重新生成构建文件
cmake -B build-cmake

# 4. 编译（只编译启用的模块）
cmake --build build-cmake
```

### 调试构建问题

```bash
# 1. 查看详细构建过程
cmake --build build-cmake --verbose

# 2. 清理并重新构建
rm -rf build-cmake
make menuconfig
cmake -B build-cmake
cmake --build build-cmake

# 3. 查看 CMake 变量
cmake -B build-cmake -LAH

# 4. 查看 Kconfig 配置
cat .config | grep CONFIG_
```

## 重要注意事项

### 配置系统

1. **先配置后构建**: 必须先运行 `make menuconfig` 生成 `.config`
2. **配置文件**: `.config` 不提交到 git，使用 `configs/*.defconfig` 管理
3. **配置变更**: 修改 `.config` 后需要重新运行 `cmake -B build-cmake`

### 构建系统

1. **不要手动修改 build-cmake/ 目录**，这是自动生成的
2. **使用 target_* 命令**，避免全局命令（如 `include_directories`）
3. **头文件统一放在 include/ 目录**，通过 `${CMAKE_SOURCE_DIR}/include` 引用
4. **库名自动添加 lib 前缀**，不需要手动指定

### 依赖关系

1. **只声明直接依赖**，CMake 会自动处理传递依赖
2. **使用 PUBLIC/PRIVATE/INTERFACE** 明确依赖的可见性
3. **不要创建循环依赖**，Core 模块之间也要注意依赖顺序

### 清理机制

```bash
# 清理构建产物
rm -rf build-cmake

# 清理配置文件
make clean

# 完全清理
rm -rf build-cmake .config include/config include/generated
```

### Git 工作流

1. **当前分支**: `feature/cmake-build-system`
2. **主分支**: `master`
3. **提交前检查**: 确保 `make menuconfig && cmake -B build-cmake && cmake --build build-cmake` 成功

## 参考文档

- [CMake 构建指南](docs/CMAKE_BUILD_GUIDE.md)
- [CMake Kconfig 集成](docs/CMAKE_KCONFIG_INTEGRATION.md)
- [架构设计](docs/ARCHITECTURE.md)
- [安装指南](docs/INSTALL.md)
- [编码规范](docs/CODING_STANDARDS.md)

## 故障排除快速参考

| 问题 | 解决方案 |
|------|---------|
| 找不到 CONFIG_XXX 变量 | 运行 `make menuconfig` 生成 `.config` |
| 找不到头文件 | 检查 `target_include_directories` 设置 |
| 链接错误 | 检查 `target_link_libraries` 依赖声明 |
| 配置不生效 | 删除 `build-cmake/` 重新配置 |
| menuconfig 无法运行 | 安装依赖：`sudo apt install libncurses-dev flex bison` |

---

**最后更新**: 2026-05-27
**维护者**: wanguo
**分支**: feature/cmake-build-system
