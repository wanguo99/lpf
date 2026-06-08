# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

EMS (Embedded Management System) 是一个采用 **Kconfig + CMake** 混合构建系统的嵌入式软件项目。

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
python3 build.py config ccm_h200_100p_am625_debug_defconfig
python3 build.py build

# 2. 运行应用
./_build/bin/ccm_collector

# 3. 运行测试（统一测试程序）
python3 build.py config tests_x86_full_defconfig  # 或其他 tests_* 配置
python3 build.py build
./_build/bin/ems-test                              # 运行所有启用的测试
```

### 构建流程

项目使用 Python 构建脚本 `build.py` 统一管理配置和编译：

```bash
# 方式一：使用 Python 构建脚本（推荐）
# 1. 加载预定义配置
python3 build.py config <config_name>    # 例如：ccm_h200_100p_am625_debug_defconfig, tests_x86_full_defconfig

# 2. 编译项目
python3 build.py build

# 3. 清理构建
python3 build.py clean      # 清理编译产物
python3 build.py distclean  # 完全清理（包括配置）

# 4. 运行测试
./_build/bin/ems-test       # 运行统一测试程序（根据 Kconfig 配置运行相应测试）

# 方式二：图形化配置（高级用户）
python3 build.py menuconfig  # 打开 Kconfig 图形界面

# 方式三：直接使用 CMake（不推荐，需要先配置 Kconfig）
cmake -B build-cmake
cmake --build build-cmake -j$(nproc)
```

**注意**：
- 推荐使用 `python3 build.py` 命令，它会自动处理 Kconfig 配置和 CMake 构建
- 构建输出目录为 `_build/`（不是 `build-cmake/`）
- 可执行文件位于 `_build/bin/`，库文件位于 `_build/lib/`

## 项目架构

### 目录结构

```
EMS/
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
├── configs/               # Kconfig 配置文件（defconfig）
├── scripts/               # 构建脚本
│   └── kconfig/          # Kconfig 配置工具
├── cmake/                 # CMake 模块
│   └── Kconfig.cmake     # Kconfig 集成模块
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

## 配置系统（Kconfig）

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

- **`.config`**: 当前配置文件（由 menuconfig 生成）
- **`configs/*_defconfig`**: 预定义配置模板
- **`Kconfig`**: 配置选项定义文件

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

### 添加新的库

1. 在 `core/mylib/Kconfig` 中添加配置选项
2. 在 `core/Kconfig` 中引用
3. 创建 `core/mylib/CMakeLists.txt`
4. 在 `core/CMakeLists.txt` 中添加子目录
5. 配置并编译

### 添加新的应用程序

1. 创建 `products/ccm/apps/myapp/CMakeLists.txt`
2. 在父目录 `CMakeLists.txt` 中添加
3. 编译

### 功能裁剪

通过 Kconfig 配置实现功能裁剪：

```bash
python3 build.py menuconfig  # 关闭不需要的模块
python3 build.py build       # 重新编译
```

### 调试构建问题

```bash
# 查看详细构建过程
python3 build.py build --verbose

# 清理并重新构建
python3 build.py distclean
python3 build.py config <config_name>
python3 build.py build

# 查看配置
cat .config | grep CONFIG_
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

| 问题 | 解决方案 |
|------|---------|
| 找不到 CONFIG_XXX 变量 | 运行 `python3 build.py config <name>` |
| 找不到头文件 | 检查 `target_include_directories` 设置 |
| 链接错误 | 检查 `target_link_libraries` 依赖声明 |
| 配置不生效 | 运行 `python3 build.py distclean` 重新配置 |
| menuconfig 无法运行 | 安装依赖：`sudo apt install libncurses-dev flex bison` |
| 编译错误 | 检查是否符合命名规范（模块前缀、枚举值等） |

## 最近更新

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

**最后更新**: 2026-06-08  
**维护者**: wanguo  
**分支**: master
