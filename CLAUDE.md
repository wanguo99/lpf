# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

ES-Middleware (Embedded Software - Middleware) 是一个采用 **Kconfig + CMake** 混合构建系统的嵌入式软件项目。

**架构总览**：参见 [系统架构文档](docs/ARCHITECTURE.md) 和 [架构图集](docs/diagrams/)

### 核心特点

- **配置系统**: Kconfig（menuconfig 图形化配置）
- **构建系统**: CMake 3.16+
- **架构模式**: Core/Products 分层架构
- **编程语言**: C 语言（C99 标准）
- **目标平台**: Linux 嵌入式系统（主要是 TI AM62x）

## 快速开始

```bash
# 1. 编译 CCM 产品（调试版本）
make ccm_h200_100p_am625_debug_defconfig
make

# 2. 运行应用
./_build/bin/ccm_collector

# 3. 运行测试（统一测试程序）
make tests_x86_full_defconfig  # 或其他 tests_* 配置
make
./_build/bin/es-middleware-test                    # 运行所有启用的测试
```

### 构建流程

项目使用内核风格的 Makefile 统一管理配置和编译：

```bash
# 1. 加载预定义配置
make tests_x86_minimal_defconfig    # 或其他配置，例如：ccm_h200_100p_am625_debug_defconfig

# 2. 编译项目（自动并行构建）
make

# 或显式指定并行度
make -j$(nproc)

# 3. 清理构建
make clean      # 清理编译产物
make distclean  # 完全清理（包括配置）

# 4. 运行测试
./_build/bin/es-middleware-test       # 运行统一测试程序（根据 Config.in 配置运行相应测试）

# 图形化配置
make menuconfig  # 打开 Kconfig ncurses 界面
make nconfig     # 替代界面

# 其他命令
make help        # 显示所有可用命令
make list        # 列出所有可用配置
```

**注意**：
- 推荐使用 `make` 命令，遵循 Linux 内核/Buildroot 风格
- 构建输出目录为 `_build/`
- 零 Python 依赖，仅需 Make + CMake + GCC + ncurses
- 可执行文件位于 `_build/bin/`，库文件位于 `_build/lib/`

## 项目架构

### 目录结构

```
ES-Middleware/
├── core/                   # 核心模块（可复用组件）
│   ├── aconfig/           # 应用配置层（业务功能到硬件设备映射）
│   ├── hal/               # 硬件抽象层（CAN、UART、I2C、SPI、GPIO 等）
│   ├── osal/              # 操作系统抽象层（线程、互斥锁、信号量等）
│   ├── pconfig/           # 平台配置层（硬件设备配置表）
│   ├── pdl/               # 外设驱动层
│   └── prl/               # 协议层（统一设备协议）
├── products/              # 产品应用（特定产品的实现）
│   └── ccm/              # CCM 产品（通信管理板）
│       ├── apps/         # 应用程序（collector、logger、health、supervisor、comm）
│       ├── libs/         # 产品库（libccm）
│       ├── osal/         # CCM 特定的 OSAL 实现
│       ├── hal/          # CCM 特定的 HAL 实现
│       ├── pdl/          # CCM 特定的 PDL 实现
│       ├── prl/          # CCM 特定的 PRL 实现
│       ├── pconfig/      # CCM 特定的 PCONFIG 实现
│       └── configs/      # 平台配置（H200 AM625）
├── tests/                 # 测试代码
│   └── core/             # 核心模块测试
├── configs/               # Config.in 配置文件（defconfig）
├── scripts/               # 构建脚本
│   └── config/          # Config.in 配置工具
├── cmake/                 # CMake 模块
│   └── Kconfig.cmake     # Config.in 集成模块
├── include/               # 公共头文件目录
├── _build/                # 构建输出目录（不提交到 git）
│   ├── bin/              # 可执行文件
│   └── lib/              # 库文件
└── docs/                  # 项目文档
```

### 模块依赖关系

```
products/ccm/apps/*  →  libccm  →  core/aconfig  →  core/pconfig  →  core/pdl  →  core/prl  →  core/hal  →  core/osal
```

**重要规则**:
- Products 依赖 Core，但 Core 不能依赖 Products
- 依赖关系通过 CMake 的 `target_link_libraries` 自动管理
- CMake 会自动处理传递依赖（transitive dependencies）

## 配置系统（Kconfig + CMake 集成）

### 架构概述

项目采用 **Kconfig + CMake 混合构建系统**，实现配置驱动的条件编译。这是一个完全原生的 CMake 集成方案，使用 Linux 内核标准的 Config.in 工具链。

```
用户操作:
  make <defconfig>  →  加载 defconfig
                  ↓
  Kconfig 工作流:
  1. conf --defconfig=<defconfig>      →  生成 .config
  2. conf --olddefconfig .config       →  规范化配置（填充派生值）
                  ↓
  CMake 配置阶段:
  3. kconfig_load()                    →  调用 genconfig.py
  4. genconfig.py .config              →  生成 kconfig.cmake + autoconf.h
  5. include(kconfig.cmake)            →  导入 CMake 变量
  6. add_compile_options(-include)     →  自动包含 autoconf.h
                  ↓
  构建阶段:
  - CMakeLists.txt 使用 CONFIG_XXX 变量控制条件编译
  - C 代码使用 autoconf.h 中的宏定义
```

**核心特性**：
- **标准工具链**: 使用 Linux 内核 Config.in 工具（conf、mconf）
- **CMake 原生**: 配置管理完全在 CMake 中完成，无需外部脚本
- **双重接口**: CMake 变量（条件编译）+ C 宏定义（代码内条件编译）
- **自动同步**: 配置变更自动触发 CMake 重新配置
- **派生值支持**: 自动填充 Kconfig 派生值（如 `CONFIG_PROJECT_NAME`）

**工作流程**：
1. `make <defconfig>` - 加载 defconfig，生成并规范化 `.config`
2. `cmake -B _build` - CMake 调用 `kconfig_load()` 生成 `kconfig.cmake` 和 `autoconf.h`
3. CMakeLists.txt 使用 `CONFIG_XXX` 变量控制库/应用的编译
4. C 代码使用 `autoconf.h` 中的宏进行条件编译
5. `make savedefconfig` - 保存当前配置为新的 defconfig

### 可用配置

#### 产品配置
- `ccm_h200_100p_am625_debug_defconfig` - CCM H200-100P-AM625 调试版本（包含所有调试功能）
- `ccm_h200_100p_am625_release_defconfig` - CCM H200-100P-AM625 发布版本（优化配置，禁用测试）

#### 测试配置
- `tests_x86_full_defconfig` - 全栈测试（所有模块、所有功能）
- `tests_x86_pdl_defconfig` - PDL 单元测试
- `tests_x86_prl_defconfig` - PRL 协议层测试
- `tests_x86_aconfig_defconfig` - ACONFIG 单元测试
- `tests_x86_pconfig_defconfig` - PCONFIG 单元测试
- `tests_x86_system_defconfig` - 系统集成测试
- `tests_x86_stress_defconfig` - 压力测试
- `tests_x86_minimal_defconfig` - 最小化配置（仅核心功能）

### 配置文件

- **`.config`**: 当前 Config.in 配置（由 menuconfig 或 defconfig 生成）
- **`configs/*_defconfig`**: 预定义配置模板
- **`Kconfig`**: 配置选项定义文件（menu 结构）
- **`_build/kconfig.cmake`**: CMake 变量文件（自动生成）
- **`_build/autoconf.h`**: C 宏定义头文件（自动生成）

### CMake 中使用配置变量

在 CMakeLists.txt 中使用 Config.in 配置：

```cmake
# 加载 Config.in 配置（在顶层 CMakeLists.txt）
include(cmake/Kconfig.cmake)

# 条件编译库
if(CONFIG_OSAL)
    add_subdirectory(core/osal)
endif()

# 检查配置值
if(CONFIG_BUILD_TESTING)
    add_subdirectory(tests)
endif()

# 使用字符串配置
set(PLATFORM_NAME ${CONFIG_PLATFORM_NAME})
```

**可用的 CMake 变量**：
- 布尔选项：`CONFIG_XXX` (ON/OFF)
- 整数选项：`CONFIG_XXX` (数字)
- 字符串选项：`CONFIG_XXX` (字符串)

### C 代码中使用配置宏

在 C 代码中使用 `autoconf.h` 中的宏：

```c
#include "autoconf.h"

/* 条件编译代码 */
#ifdef CONFIG_OSAL_DEBUG
    printf("Debug mode enabled\n");
#endif

/* 使用配置值 */
#if CONFIG_MAX_THREADS > 10
    #warning "Thread pool may be too large"
#endif

/* 字符串配置 */
const char *platform = CONFIG_PLATFORM_NAME;
```

**注意**：
- `autoconf.h` 自动包含在所有源文件中（通过 `-include` 编译选项）
- 布尔配置生成 `#define CONFIG_XXX 1` 或不定义
- 字符串配置生成 `#define CONFIG_XXX "value"`

### 常用配置选项

```kconfig
# 核心模块开关
CONFIG_OSAL=y              # 操作系统抽象层
CONFIG_HAL=y               # 硬件抽象层
CONFIG_PCONFIG=y           # 平台配置层
CONFIG_PDL=y               # 外设驱动层
CONFIG_PRL=y               # 协议层
CONFIG_ACONFIG=y           # 应用配置层

# PRL 协议层（按设备类型组织）
CONFIG_PRL_MCU=y           # MCU 设备协议
CONFIG_PRL_CCM=y           # CCM 设备协议
CONFIG_PRL_PMC=y           # PMC 设备协议
CONFIG_PRL_GSC=y           # GSC 设备协议
CONFIG_PRL_POWER=y         # POWER 设备协议

# 平台配置
CONFIG_ARCH_X86_64=y       # x86_64 架构
CONFIG_ARCH_ARM64=y        # ARM64 架构
CONFIG_OS_LINUX=y          # Linux 操作系统

# 功能裁剪
CONFIG_BUILD_TESTING=y     # 构建测试程序
CONFIG_BUILD_SHARED=y      # 构建共享库
```

### CMake 函数参考

项目提供的 Config.in 集成函数（`cmake/Kconfig.cmake`）：

#### `kconfig_load()`

加载 Config.in 配置并生成 CMake 变量和 C 头文件。

```cmake
# 在顶层 CMakeLists.txt 中调用
include(cmake/Kconfig.cmake)
kconfig_load()
```

**功能**：
1. **检查 .config 存在性** - 如果不存在，提示运行 `python3 build.py config`
2. **生成 CMake 变量文件** - 调用 `genconfig.py` 生成 `_build/kconfig.cmake`
3. **生成 C 头文件** - 生成 `_build/autoconf.h` 供 C 代码使用
4. **导入 CMake 变量** - `include(kconfig.cmake)` 使所有 `CONFIG_*` 变量在 CMake 中可用
5. **配置自动包含** - 添加 `-include autoconf.h` 编译选项，使所有 C 文件自动包含配置宏

**调用时机**：
- 必须在顶层 `CMakeLists.txt` 的 `project()` 命令**之后**立即调用
- 在任何使用 `CONFIG_*` 变量的条件判断之前调用

**自动触发机制**：
- CMake 会自动检测 `.config` 文件变化
- 配置修改后，下次构建会自动重新生成 `kconfig.cmake` 和 `autoconf.h`
- 无需手动清理 CMake 缓存

**错误处理**：
- 如果 `.config` 不存在，CMake 配置失败并显示错误消息
- 如果 `genconfig.py` 执行失败，显示详细错误信息和诊断建议

**示例**：
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(ES-Middleware C)

# 加载 Config.in 配置（必须在 project() 之后）
include(cmake/Kconfig.cmake)
kconfig_load()

# 现在可以使用 CONFIG_* 变量
if(CONFIG_OSAL)
    add_subdirectory(core/osal)
endif()

if(CONFIG_BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
```

#### `kconfig_print_summary()`

打印当前配置摘要（用于调试）。

```cmake
kconfig_print_summary()
```

**输出示例**：
```
-- Kconfig Configuration Summary:
--   CONFIG_PROJECT_NAME: ES-Middleware
--   CONFIG_OSAL: ON
--   CONFIG_HAL: ON
--   CONFIG_PDL: ON
--   CONFIG_PRL: ON
--   CONFIG_ACONFIG: ON
--   CONFIG_BUILD_TESTING: OFF
--   CONFIG_ARCH_X86_64: ON
--   CONFIG_BUILD_TYPE: debug
--   ... (total 110+ symbols)
```

**使用场景**：
- 调试配置问题时验证符号是否正确加载
- 开发新模块时确认依赖的配置选项已启用
- CI/CD 流程中记录构建配置

**调用位置**：
```cmake
kconfig_load()
kconfig_print_summary()  # 可选，仅在需要时调用
```

### 配置管理最佳实践

#### 1. 创建新的 defconfig

```bash
# 方式一：基于现有配置修改
make tests_x86_full_defconfig
make menuconfig  # 修改配置
make savedefconfig my_new_defconfig

# 方式二：从头开始配置
make menuconfig  # 从默认配置开始
make savedefconfig my_new_defconfig

# 方式三：手动编辑（高级用户）
cp configs/tests_x86_full_defconfig configs/my_new_defconfig
# 编辑 configs/my_new_defconfig，只保留非默认值
make my_new_defconfig  # 验证配置可加载
```

**defconfig 文件格式**：
- 只包含**用户显式设置的选项**（非默认值）
- 不包含派生值（如 `CONFIG_PROJECT_NAME`）
- 不包含依赖自动禁用的选项
- 格式：`CONFIG_OPTION=y` 或 `# CONFIG_OPTION is not set`

**示例 defconfig**：
```kconfig
CONFIG_OSAL=y
CONFIG_HAL=y
CONFIG_PDL=y
CONFIG_PRL=y
CONFIG_PRL_MCU=y
CONFIG_PRL_CCM=y
CONFIG_BUILD_TESTING=y
CONFIG_ARCH_X86_64=y
CONFIG_BUILD_TYPE="debug"
```

#### 2. 修改现有配置

```bash
# 加载配置
make ccm_h200_100p_am625_debug_defconfig

# 修改配置（图形界面）
make menuconfig

# 保存配置（覆盖原 defconfig）
make savedefconfig ccm_h200_100p_am625_debug_defconfig

# 或保存为新配置
make savedefconfig my_variant_defconfig
```

**注意事项**：
- `savedefconfig` 会自动最小化配置（只保存非默认值）
- 派生值和依赖关系会在加载时自动恢复
- 提交前请验证配置可以成功构建

#### 3. 查看配置差异

```bash
# 当前 .config 与原始 defconfig 的差异
diff .config configs/tests_x86_full_defconfig

# 查看哪些选项被修改
make tests_x86_full_defconfig
cp .config .config.original
make menuconfig  # 修改一些选项
diff .config.original .config

# 查看生成的 CMake 变量
cat _build/kconfig.cmake | grep CONFIG_

# 查看生成的 C 宏定义
cat _build/autoconf.h | grep CONFIG_
```

#### 4. 调试配置问题

```bash
# 问题：某个选项无法启用
make menuconfig
# 导航到该选项，按 '?' 查看帮助和依赖关系

# 问题：配置加载后某些选项消失
# 原因：依赖条件不满足
# 解决：检查 Config.in 文件中的 "depends on" 语句

# 问题：CMake 变量不可用
# 诊断步骤：
cat .config | grep CONFIG_XXX              # 检查是否在 .config 中
cat _build/kconfig.cmake | grep CONFIG_XXX # 检查是否生成 CMake 变量
grep "kconfig_load" CMakeLists.txt         # 检查是否调用加载函数

# 问题：C 宏定义不可用
cat _build/autoconf.h | grep CONFIG_XXX    # 检查是否生成宏定义
# 检查编译命令是否包含 -include
make -- VERBOSE=1 | grep "include.*autoconf.h"

# 清理并重新配置
make distclean
make <defconfig>
make
```

#### 5. 配置验证和测试

```bash
# 验证所有 defconfig 可以加载
for cfg in configs/*_defconfig; do
    echo "Testing $(basename $cfg)..."
    make distclean
    make $(basename $cfg) || echo "FAILED: $cfg"
done

# 验证配置可以构建
make tests_x86_full_defconfig
make || echo "Build failed"

# 验证 savedefconfig 幂等性（加载-保存-加载应该一致）
make tests_x86_full_defconfig
make savedefconfig /tmp/test.defconfig
make distclean
make /tmp/test.defconfig
diff .config <original_config>  # 应该只有注释差异
```

#### 6. 高级配置技巧

**查看所有可配置选项**：
```bash
make menuconfig
# 按 '/' 打开搜索，输入关键字
# 按 '?' 查看选项的详细信息和依赖关系
```

**批量启用/禁用选项**：
```bash
# 手动编辑 .config
make base_defconfig
# 编辑 .config，添加或修改选项
make  # CMake 会自动检测变化并重新配置
```

**使用环境变量覆盖**（不推荐，仅用于快速测试）：
```bash
# 在 CMakeLists.txt 中可以检查环境变量
if(DEFINED ENV{FORCE_DEBUG})
    set(CMAKE_BUILD_TYPE Debug)
endif()
```

**配置模板继承**（手动实现）：
```bash
# 创建基础配置
cat > configs/base_defconfig << EOF
CONFIG_OSAL=y
CONFIG_HAL=y
EOF

# 创建扩展配置
cat configs/base_defconfig > configs/extended_defconfig
cat >> configs/extended_defconfig << EOF
CONFIG_PDL=y
CONFIG_BUILD_TESTING=y
EOF
```

## 编码规范

### 命名规范

详细规范请参考 `docs/NAMING_CONVENTIONS.md`，核心要点：

#### 对外 API
- 使用**大写**模块前缀：`PRL_Encode()`, `PDL_MCU_Init()`, `ACONFIG_Init()`
- 格式：`MODULE_VerbNoun()` 或 `MODULE_Noun_Verb()`

#### 内部函数
- 使用**小写**模块前缀：`prl_init_header()`, `pdl_mcu_send_command()`
- 静态函数可省略模块前缀

#### 类型定义
- 使用 `module_name_t` 格式：`prl_header_t`, `pdl_mcu_handle_t`

#### 宏定义
- 使用 `MODULE_NAME` 格式：`PRL_MAX_PACKET_SIZE`, `ACONFIG_TC_POWER_ON`

#### 枚举值
- 必须包含模块前缀：`ACONFIG_TC_POWER_ON`（不是 `TC_POWER_ON`）

#### 头文件保护宏
- 必须包含模块前缀：`HAL_CAN_TYPES_H`（不是 `CAN_TYPES_H`）

### C 语言规范

- **标准**: C99（支持 `//` 注释和现代 C 特性）
- **风格**: Linux 内核编码风格
- **缩进**: Tab（8 空格宽度）
- **命名**: 小写加下划线（snake_case）

### 头文件引用规范

**所有头文件引用不使用命名空间前缀**，直接使用文件名：

```c
/* 正确 ✓ */
#include "osal.h"
#include "hal_can.h"
#include "pdl.h"
#include "prl_api.h"
#include "pconfig.h"
#include "aconfig.h"

/* 错误 ✗ - 不要使用命名空间前缀 */
#include "osal/osal.h"
#include "hal/hal_can.h"
#include "pdl/pdl.h"
```

**说明**：
- CMakeLists.txt 已配置正确的头文件搜索路径
- 便于 IDE 查找和跳转头文件
- 适用于所有核心模块：OSAL、HAL、PDL、PRL、PCONFIG、ACONFIG

### POSIX 特性宏

项目自动定义以下宏以启用 POSIX 扩展：

```c
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define _GNU_SOURCE
```

## PRL 协议层架构

### 设计理念

PRL 采用**统一协议**设计，所有内部设备使用相同的协议格式：

- 协议头中的 `dev_type` 字段区分设备类型
- 任何设备都可以与任何设备通信
- 按设备类型组织（不是点对点）

### 协议头结构（20 字节）

```c
typedef struct {
    uint16_t magic;         /* 魔数：0xAA55 */
    uint8_t  version;       /* 协议版本 */
    uint8_t  dev_type;      /* 设备类型（区分设备）*/
    uint8_t  msg_type;      /* 消息类型（设备特定） */
    uint8_t  flags;         /* 标志位 */
    uint16_t length;        /* 负载长度 */
    uint32_t seq;           /* 序列号 */
    uint32_t timestamp;     /* 时间戳 */
    uint16_t crc16;         /* CRC16 校验 */
    uint16_t reserved;      /* 保留 */
} prl_header_t;
```

### 设备类型

- `PRL_DEV_TYPE_MCU` - 微控制器
- `PRL_DEV_TYPE_CCM` - 通信管理板
- `PRL_DEV_TYPE_PMC` - 载荷管理器
- `PRL_DEV_TYPE_GSC` - 地面站控制器
- `PRL_DEV_TYPE_POWER` - 电源板
- `PRL_DEV_TYPE_SATELLITE` - 卫星平台（不使用 PRL）
- `PRL_DEV_TYPE_BMC` - 基板管理控制器（不使用 PRL）

### 文件组织

按设备类型组织（v1.2 重构后）：

```
core/prl/
├── include/
│   ├── prl.h              # 对外统一头文件（推荐使用）
│   ├── prl_common.h       # 通用定义（内部）
│   ├── prl_device.h       # 设备消息定义（内部）
│   ├── prl_mcu.h          # MCU 设备协议
│   ├── prl_ccm.h          # CCM 设备协议
│   ├── prl_pmc.h          # PMC 设备协议
│   ├── prl_gsc.h          # GSC 设备协议
│   ├── prl_power.h        # POWER 设备协议
│   └── prl_pmc_ccm.h      # 兼容层（已废弃）
└── src/
    ├── prl_api.c          # 对外 API 实现
    ├── prl_common.c       # 通用实现
    ├── prl_device.c       # 设备消息实现
    ├── prl_mcu.c          # MCU 协议实现
    ├── prl_ccm.c          # CCM 协议实现
    ├── prl_pmc.c          # PMC 协议实现
    ├── prl_gsc.c          # GSC 协议实现
    └── prl_power.c        # POWER 协议实现
```

### PRL API 使用

```c
#include "prl.h"

/* 编码消息 */
uint8_t buffer[PRL_MAX_PACKET_SIZE];
int len = PRL_Encode(PRL_DEV_TYPE_MCU,      /* 设备类型 */
                     PRL_MCU_MSG_GET_VERSION, /* 消息类型 */
                     &payload, sizeof(payload),
                     buffer, sizeof(buffer), 0);

/* 解码消息 */
uint8_t dev_type, msg_type;
const uint8_t *payload;
uint16_t payload_len;
int ret = PRL_Decode(buffer, len, &dev_type, &msg_type,
                     &payload, &payload_len);
```

## 常见开发任务

### 添加新的核心库模块

完整流程示例：添加一个新的 `core/utils` 模块

**1. 创建目录结构**
```bash
mkdir -p core/utils/include/utils
mkdir -p core/utils/src
```

**2. 添加 Config.in 配置**
```bash
# 创建 core/utils/Kconfig
cat > core/utils/Kconfig << 'EOF'
config UTILS
    bool "Utility functions"
    default y
    help
      Common utility functions for string manipulation,
      data conversion, etc.

if UTILS

config UTILS_STRING
    bool "String utilities"
    default y
    help
      String manipulation functions.

config UTILS_MATH
    bool "Math utilities"
    default y
    help
      Math helper functions.

endif # UTILS
EOF

# 在 core/Kconfig 中引用
# 编辑 core/Kconfig，在合适位置添加：
source "core/utils/Kconfig"
```

**3. 创建 CMakeLists.txt**
```cmake
# core/utils/CMakeLists.txt
if(NOT CONFIG_UTILS)
    return()
endif()

# 定义库
add_library(utils
    src/utils_common.c
)

# 条件编译可选组件
if(CONFIG_UTILS_STRING)
    target_sources(utils PRIVATE src/utils_string.c)
endif()

if(CONFIG_UTILS_MATH)
    target_sources(utils PRIVATE src/utils_math.c)
endif()

# 头文件路径
target_include_directories(utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 依赖其他库（如果需要）
target_link_libraries(utils
    PUBLIC osal  # 如果依赖 OSAL
)

# 安装规则（可选）
install(TARGETS utils
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
)
```

**4. 在父 CMakeLists.txt 中添加**
```cmake
# 编辑 core/CMakeLists.txt，添加：
add_subdirectory(utils)
```

**5. 创建头文件**
```c
/* core/utils/include/utils/utils.h */
#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* 公共 API */
int UTILS_Init(void);
void UTILS_Cleanup(void);

#ifdef CONFIG_UTILS_STRING
int UTILS_String_Copy(char *dst, const char *src, size_t size);
#endif

#ifdef CONFIG_UTILS_MATH
int UTILS_Math_Clamp(int value, int min, int max);
#endif

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */
```

**6. 实现源文件**
```c
/* core/utils/src/utils_common.c */
#include "utils/utils.h"
#include "autoconf.h"  /* 自动包含，可以使用 CONFIG_* 宏 */

int UTILS_Init(void)
{
#ifdef CONFIG_UTILS_STRING
    /* 初始化字符串模块 */
#endif
    return 0;
}
```

**7. 配置并构建**
```bash
# 启用新模块
make menuconfig
# 导航到 "Core Components" -> "Utility functions"
# 按 'Y' 启用

# 或修改 defconfig
echo "CONFIG_UTILS=y" >> configs/tests_x86_full_defconfig

# 重新构建
make tests_x86_full_defconfig
make
```

**8. 在其他模块中使用**
```cmake
# 在 CMakeLists.txt 中声明依赖
target_link_libraries(myapp PRIVATE utils)
```

```c
/* 在 C 代码中使用 */
#include "utils/utils.h"

int main(void)
{
    UTILS_Init();
    /* 使用工具函数 */
    UTILS_Cleanup();
    return 0;
}
```

### 添加新的产品应用程序

示例：在 CCM 产品下添加新的应用程序

**1. 创建应用目录**
```bash
mkdir -p products/ccm/apps/myapp
```

**2. 添加 Config.in 配置**
```bash
# 编辑 products/ccm/apps/Kconfig
config CCM_APP_MYAPP
    bool "MyApp application"
    depends on CCM
    default n
    help
      Description of MyApp.
```

**3. 创建 CMakeLists.txt**
```cmake
# products/ccm/apps/myapp/CMakeLists.txt
if(NOT CONFIG_CCM_APP_MYAPP)
    return()
endif()

add_executable(ccm_myapp
    main.c
    myapp_task.c
)

# 链接依赖库
target_link_libraries(ccm_myapp PRIVATE
    ccm          # CCM 产品库
    aconfig      # 应用配置层
    pconfig      # 平台配置层
    pdl          # 外设驱动层
    prl          # 协议层
    hal          # 硬件抽象层
    osal         # 操作系统抽象层
)

# 头文件路径
target_include_directories(ccm_myapp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 安装
install(TARGETS ccm_myapp
    RUNTIME DESTINATION bin
)
```

**4. 在父 CMakeLists.txt 中添加**
```cmake
# 编辑 products/ccm/apps/CMakeLists.txt，添加：
add_subdirectory(myapp)
```

**5. 实现应用程序**
```c
/* products/ccm/apps/myapp/main.c */
#include "autoconf.h"
#include "osal.h"
#include "ccm.h"

int main(int argc, char *argv[])
{
    /* 初始化 */
    OSAL_Init();
    CCM_Init();
    
    /* 应用逻辑 */
    printf("MyApp running...\n");
    
    /* 清理 */
    CCM_Cleanup();
    OSAL_Cleanup();
    
    return 0;
}
```

**6. 配置并构建**
```bash
make menuconfig
# 导航到 "Products" -> "CCM Applications" -> "MyApp"
# 启用后保存

make
./_build/bin/ccm_myapp
```

### 添加单元测试

示例：为新模块添加单元测试

**1. 创建测试目录**
```bash
mkdir -p tests/core/utils
```

**2. 添加 Config.in 配置**
```bash
# 编辑 tests/core/Kconfig
config TEST_UTILS
    bool "UTILS module tests"
    depends on BUILD_TESTING && UTILS
    default y if BUILD_TESTING
    help
      Unit tests for UTILS module.
```

**3. 创建测试代码**
```c
/* tests/core/utils/test_utils.c */
#include "utils/utils.h"
#include <assert.h>
#include <stdio.h>

void test_utils_string(void)
{
    char buf[32];
    int ret;
    
    ret = UTILS_String_Copy(buf, "hello", sizeof(buf));
    assert(ret == 0);
    assert(strcmp(buf, "hello") == 0);
    
    printf("test_utils_string: PASSED\n");
}

void test_utils_math(void)
{
    assert(UTILS_Math_Clamp(5, 0, 10) == 5);
    assert(UTILS_Math_Clamp(-5, 0, 10) == 0);
    assert(UTILS_Math_Clamp(15, 0, 10) == 10);
    
    printf("test_utils_math: PASSED\n");
}

int main(void)
{
    printf("Running UTILS tests...\n");
    
    test_utils_string();
    test_utils_math();
    
    printf("All UTILS tests passed!\n");
    return 0;
}
```

**4. 创建 CMakeLists.txt**
```cmake
# tests/core/utils/CMakeLists.txt
if(NOT CONFIG_TEST_UTILS)
    return()
endif()

add_executable(test_utils
    test_utils.c
)

target_link_libraries(test_utils PRIVATE
    utils
    osal
)

# 注册 CTest
add_test(NAME test_utils
    COMMAND test_utils
)
```

**5. 在父 CMakeLists.txt 中添加**
```cmake
# 编辑 tests/core/CMakeLists.txt，添加：
add_subdirectory(utils)
```

**6. 运行测试**
```bash
# 配置测试
make tests_x86_full_defconfig
make

# 运行所有测试
cd _build
ctest --output-on-failure

# 或单独运行
./_build/bin/test_utils
```

### 功能裁剪（减小二进制大小）

通过 Config.in 配置实现功能裁剪：

**1. 创建最小化配置**
```bash
make menuconfig

# 禁用不需要的模块：
# [ ] Build testing          # 禁用测试
# [ ] PRL - PMC protocol     # 禁用不需要的设备协议
# [ ] PRL - GSC protocol
# [ ] Debug logging          # 禁用调试日志

# 保存配置
make savedefconfig minimal_defconfig
```

**2. 优化编译选项**
```cmake
# 在 CMakeLists.txt 中添加
if(CONFIG_BUILD_TYPE STREQUAL "release")
    add_compile_options(-Os)           # 优化代码大小
    add_compile_options(-ffunction-sections)
    add_compile_options(-fdata-sections)
    add_link_options(-Wl,--gc-sections) # 移除未使用的段
    add_compile_definitions(NDEBUG)     # 禁用 assert
endif()
```

**3. 条件编译功能**
```c
/* 在代码中根据配置裁剪功能 */
#ifdef CONFIG_DEBUG_LOGGING
    printf("Debug: %s\n", msg);
#endif

#ifdef CONFIG_PRL_PMC
    prl_pmc_init();
#endif
```

**4. 比较二进制大小**
```bash
# 完整配置
make tests_x86_full_defconfig
make
ls -lh _build/bin/ccm_collector

# 最小化配置
make minimal_defconfig
make
ls -lh _build/bin/ccm_collector

# 分析符号大小
nm --size-sort --radix=d _build/bin/ccm_collector | tail -20
```

### 交叉编译（ARM64 目标）

为嵌入式目标平台交叉编译：

**1. 准备工具链**
```bash
# 安装交叉编译工具链
sudo apt install gcc-aarch64-linux-gnu
```

**2. 创建工具链文件**
```cmake
# cmake/toolchain-arm64.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

**3. 配置交叉编译**
```bash
# 加载 ARM64 配置
make ccm_h200_100p_am625_release_defconfig

# 使用工具链文件构建
cmake -B _build-arm64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm64.cmake
cmake --build _build-arm64 -j$(nproc)

# 检查二进制架构
file _build-arm64/bin/ccm_collector
# 输出：ELF 64-bit LSB executable, ARM aarch64, ...
```

**4. 部署到目标**
```bash
# 通过 scp 传输
scp _build-arm64/bin/ccm_collector root@target:/usr/bin/

# 或打包为 tarball
tar -czf ccm-arm64.tar.gz -C _build-arm64/bin .
```

### 调试构建问题

**详细构建输出**：
```bash
make -- VERBOSE=1
```

**检查链接顺序**：
```bash
make -- VERBOSE=1 2>&1 | grep "undefined reference"
```

**检查头文件搜索路径**：
```bash
make -- VERBOSE=1 2>&1 | grep "\-I"
```

**检查库链接**：
```bash
make -- VERBOSE=1 2>&1 | grep "\-l"
```

**生成依赖关系图**：
```bash
cd _build
cmake --graphviz=deps.dot ..
dot -Tpng deps.dot -o deps.png
```

## 重要注意事项

### 配置系统

1. **先配置后构建**: 必须先加载配置或运行 menuconfig
2. **配置文件**: `.config` 不提交到 git，使用 `configs/*_defconfig` 管理
3. **配置变更**: 修改配置后需要重新构建

### 构建系统

1. **不要手动修改 _build/ 目录**，这是自动生成的
2. **使用 target_* 命令**，避免全局命令（如 `include_directories`）
3. **头文件统一放在 include/ 目录**
4. **库名自动添加 lib 前缀**，不需要手动指定

### 依赖关系

1. **只声明直接依赖**，CMake 会自动处理传递依赖
2. **使用 PUBLIC/PRIVATE/INTERFACE** 明确依赖的可见性
3. **不要创建循环依赖**

### Git 工作流

1. **当前分支**: `master`
2. **提交前检查**: 确保至少一个配置编译成功
3. **Commit message**: 使用规范的提交信息格式

## 参考文档

- [命名规范](docs/NAMING_CONVENTIONS.md) - **必读**
- [PRL 架构设计](docs/PRL_ARCHITECTURE.md) - 协议层详解
- [PRL 使用指南](docs/PRL_USAGE_GUIDE.md) - API 使用示例
- [CMake 构建指南](docs/CMAKE_BUILD_GUIDE.md)
- [架构设计](docs/ARCHITECTURE.md)
- [编码规范](docs/CODING_STANDARDS.md)

## 故障排除快速参考

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 找不到 CONFIG_XXX CMake 变量 | `.config` 未加载或未生成 | 运行 `make <name>` |
| CONFIG_XXX 在 C 代码中未定义 | `autoconf.h` 未生成 | 检查 `_build/autoconf.h` 是否存在，重新运行 CMake |
| 配置修改后不生效 | CMake 缓存未更新 | 运行 `make distclean && make <name>` |
| kconfig.cmake 生成失败 | Python 脚本错误 | 检查 `tools/config/genconfig.py` 输出，确认 `.config` 格式正确 |
| menuconfig 无法运行 | 缺少 ncurses 依赖 | 安装依赖：`sudo apt install libncurses-dev flex bison` |
| CMake 配置阶段报错 | Config.in 集成失败 | 检查 CMake 输出，确认 `kconfig_load()` 成功执行 |
| 找不到头文件 | `target_include_directories` 未设置 | 检查模块 CMakeLists.txt 的头文件路径配置 |
| 链接错误 | 依赖库未链接 | 检查 `target_link_libraries` 依赖声明 |
| 编译错误（命名冲突） | 未遵循命名规范 | 检查模块前缀、枚举值、头文件保护宏 |
| defconfig 加载后符号丢失 | 依赖关系未满足 | 使用 menuconfig 查看符号依赖，启用必需的父选项 |

### Kconfig + CMake 集成常见问题

#### 问题：CMake 变量 CONFIG_XXX 不可用

**症状**：CMakeLists.txt 中 `if(CONFIG_XXX)` 总是为假，或提示变量未定义

**诊断步骤**：
```bash
# 1. 检查 .config 是否存在
ls -la .config

# 2. 检查符号是否在 .config 中
cat .config | grep CONFIG_XXX

# 3. 检查 kconfig.cmake 是否生成
ls -la _build/kconfig.cmake
cat _build/kconfig.cmake | grep CONFIG_XXX

# 4. 检查 CMake 是否加载了 kconfig.cmake
grep "kconfig_load" CMakeLists.txt
```

**可能原因和解决方案**：

1. **未运行配置命令**
   ```bash
   make ccm_h200_100p_am625_debug_defconfig
   ```

2. **CMakeLists.txt 未调用 kconfig_load()**
   ```cmake
   # 确保顶层 CMakeLists.txt 包含
   include(cmake/Kconfig.cmake)
   kconfig_load()
   ```

3. **派生符号未生成**（如 `CONFIG_PROJECT_NAME`）
   - 原因：defconfig 只包含用户选项，不包含派生值
   - 解决：`build.py config` 已自动运行 `conf --olddefconfig` 规范化配置
   - 验证：`cat .config | grep CONFIG_PROJECT_NAME` 应该有值

4. **CMake 缓存问题**
   ```bash
   make distclean
   make <defconfig>
   make
   ```

#### 问题：C 代码中 CONFIG_XXX 宏未定义

**症状**：编译错误 `'CONFIG_XXX' undeclared` 或条件编译不生效

**诊断步骤**：
```bash
# 1. 检查 autoconf.h 是否生成
cat _build/autoconf.h | grep CONFIG_XXX

# 2. 检查编译命令是否包含 -include
make -- VERBOSE=1 2>&1 | grep "include.*autoconf.h"

# 3. 手动预处理测试
gcc -E -include _build/autoconf.h test.c | grep CONFIG_XXX
```

**可能原因和解决方案**：

1. **autoconf.h 未生成**
   ```bash
   # 重新生成
   make distclean
   make <defconfig>
   make
   ```

2. **编译选项未配置**
   - `kconfig_load()` 应该自动添加 `-include` 选项
   - 手动检查：`add_compile_options(-include ${CMAKE_BINARY_DIR}/autoconf.h)`

3. **符号在 .config 中不存在**
   ```bash
   # 检查符号是否启用
   cat .config | grep CONFIG_XXX
   # 如果没有，使用 menuconfig 启用
   make menuconfig
   ```

#### 问题：配置更改后编译结果不变

**症状**：修改 .config 或 defconfig 后，编译产物没有变化

**诊断步骤**：
```bash
# 1. 检查 .config 时间戳
ls -l .config

# 2. 检查 kconfig.cmake 时间戳
ls -l _build/kconfig.cmake

# 3. 检查 CMake 是否检测到变化
make -- VERBOSE=1 2>&1 | grep "Re-run.*genconfig"
```

**解决方案**：

1. **强制重新配置**
   ```bash
   # 删除 CMake 缓存
   rm -rf _build/CMakeCache.txt _build/CMakeFiles
   make
   ```

2. **完全清理重建**
   ```bash
   make distclean
   make <defconfig>
   make
   ```

3. **确认配置已保存**
   ```bash
   # 修改后必须保存
   make menuconfig  # 修改后按 'S' 保存
   # 或
   make savedefconfig <name>
   ```

**注意**：CMake 会自动检测 `.config` 的 mtime 变化。如果配置没有生效，说明 CMake 缓存了旧值。

#### 问题：genconfig.py 生成失败

**症状**：CMake 配置阶段报错 `genconfig.py failed with exit code X`

**诊断步骤**：
```bash
# 1. 手动运行生成脚本
python3 tools/config/genconfig.py .config _build/kconfig.cmake _build/autoconf.h

# 2. 检查 Python 环境
python3 --version  # 应该 >= 3.6

# 3. 检查 .config 格式
file .config       # 应该是文本文件
head -20 .config   # 检查是否有语法错误
```

**可能原因和解决方案**：

1. **Python 版本过低**
   ```bash
   # 需要 Python 3.6+
   python3 --version
   # 如果版本太低，升级 Python
   ```

2. **.config 格式错误**
   ```bash
   # 重新生成配置
   make distclean
   make <defconfig>
   ```

3. **genconfig.py 文件损坏**
   ```bash
   # 检查文件完整性
   python3 -m py_compile tools/config/genconfig.py
   ```

4. **权限问题**
   ```bash
   # 检查写权限
   ls -l _build/
   chmod -R u+w _build/
   ```

#### 问题：符号依赖关系不满足

**症状**：menuconfig 中某个选项是灰色的，无法启用；或者 defconfig 加载后某些选项消失

**诊断步骤**：
```bash
# 1. 使用 menuconfig 查看依赖
make menuconfig
# 导航到符号，按 '?' 查看帮助

# 示例输出：
# Symbol: CONFIG_PRL_MCU [=n]
# Type  : bool
# Prompt: MCU device protocol support
# Depends on: CONFIG_PRL [=n]
#   -> PRL is not enabled, so PRL_MCU cannot be enabled

# 2. 检查 Kconfig 定义
grep -A 10 "config PRL_MCU" core/prl/Kconfig
```

**解决方案**：

1. **启用依赖的父选项**
   ```bash
   make menuconfig
   # 启用 CONFIG_PRL，然后 CONFIG_PRL_MCU 就可以启用了
   ```

2. **修改 defconfig 包含依赖**
   ```bash
   # 编辑 configs/my_defconfig
   CONFIG_PRL=y        # 父选项
   CONFIG_PRL_MCU=y    # 子选项
   ```

3. **检查 select 语句**
   - 有些选项会自动 `select` 其他选项
   - 使用 menuconfig 的 '?' 查看 "Selected by" 信息

**常见依赖关系**：
```
CONFIG_PRL_MCU  depends on  CONFIG_PRL
CONFIG_PRL      depends on  CONFIG_HAL
CONFIG_HAL      depends on  CONFIG_OSAL
```

#### 问题：构建失败但配置看起来正确

**症状**：CMake 配置成功，但编译失败

**诊断步骤**：
```bash
# 1. 查看详细编译输出
make -- VERBOSE=1

# 2. 检查链接错误
make 2>&1 | grep "undefined reference"

# 3. 检查是否缺少依赖库
ldd _build/bin/ccm_collector
```

**可能原因和解决方案**：

1. **依赖库未启用**
   ```cmake
   # 检查 CMakeLists.txt
   if(CONFIG_PDL)
       target_link_libraries(myapp PRIVATE pdl)
   endif()
   # 确保 CONFIG_PDL=y
   ```

2. **头文件路径未配置**
   ```cmake
   target_include_directories(mylib PUBLIC
       $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   )
   ```

3. **循环依赖**
   ```bash
   # 检查链接顺序
   make -- VERBOSE=1 2>&1 | grep "undefined reference"
   # 调整 CMakeLists.txt 中的链接顺序
   ```

#### 问题：多配置切换问题

**症状**：在不同配置之间切换后，构建出错

**解决方案**：

```bash
# 推荐：切换配置前完全清理
make distclean
make new_defconfig
make

# 或：使用不同的构建目录（高级用户）
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-debug
cmake --build build-release
```

#### 问题：并行构建失败

**症状**：`make -j8` 或 `cmake --build . -j8` 失败，但单线程构建成功

**可能原因**：
- 依赖关系未正确声明
- 竞态条件（同时写入同一文件）

**解决方案**：
```cmake
# 确保依赖声明完整
add_library(mylib src1.c src2.c)
target_link_libraries(myapp PRIVATE mylib)  # 声明依赖

# 避免自定义命令的竞态
add_custom_command(OUTPUT file.h
    COMMAND generate_header.sh
    DEPENDS input.txt
)
add_custom_target(gen_header DEPENDS file.h)
add_dependencies(mylib gen_header)  # 确保顺序
```

## 最近更新

- **2026-06-11**: Kconfig + CMake 集成架构完成
  - **重大升级**：从 Python 脚本生成配置文件迁移到 CMake 原生集成
  - **新增功能**：
    - `kconfig_load()` CMake 函数：自动加载配置、生成变量、配置编译选项
    - `kconfig_print_summary()` CMake 函数：调试配置问题的辅助工具
    - `genconfig.py` 工具：将 `.config` 转换为 `kconfig.cmake` 和 `autoconf.h`
    - 配置规范化：自动运行 `conf --olddefconfig` 填充派生值
  - **工作流改进**：
    - 修复 `CONFIG_PROJECT_NAME` 等派生符号未生成的问题
    - 配置变更自动触发 CMake 重新配置
    - 110+ Kconfig 符号正确传递到 CMake 和 C 代码
  - **验证测试**：
    - 所有 12 个 defconfig 配置验证通过
    - 28 个自动化测试全部通过
    - 构建、保存、加载配置的完整流程验证
  - **文档完善**：
    - 详细的架构说明和工作流程图
    - CMake 函数 API 参考
    - 配置管理最佳实践
    - 常见问题诊断和解决方案
    - 开发任务示例（添加模块、应用、测试）
  - **向后兼容**：保持 `python3 build.py` 命令接口不变，内部实现升级为 CMake 原生

- **2026-06-05**: 头文件引用规范更新
  - 移除所有头文件 include 的命名空间前缀
  - 统一使用直接文件名引用（如 `#include "osal.h"` 而非 `#include "osal/osal.h"`）
  - 更新所有核心模块 CMakeLists.txt 的头文件搜索路径
  - 方便 IDE 查找和跳转头文件
  - 涉及 197 个文件修改

- **2026-06-05**: 统一头文件命名
  - PDL 新增 `pdl.h` 统一头文件（参考 OSAL）
  - PRL 使用 `prl.h` 作为对外统一头文件

- **2026-06-01**: PRL 协议层重构完成（v1.2）
  - 从点对点协议改为按设备类型组织
  - 统一命名规范（对外 API 使用 `PRL_` 前缀）
  - 新增 14 个场景化 defconfig 配置
  - 完善文档体系

- **2026-06-01**: 命名规范统一
  - 对外 API 使用大写前缀
  - 内部函数使用小写前缀
  - 枚举值必须包含模块前缀
  - 头文件保护宏必须包含模块前缀

---

**最后更新**: 2026-06-11  
**维护者**: wanguo  
**分支**: master
