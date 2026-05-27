# CMake 与 Kconfig 集成方案

**文档版本**: 1.0  
**创建日期**: 2026-05-27  
**目标**: 在 CMake 构建系统中保留 Kconfig 配置功能

---

## 执行摘要

**答案**: ✅ **可以**，CMake 可以与 Kconfig 集成，有多种方案可选。

### 方案对比

| 方案 | 复杂度 | Kconfig 兼容性 | 推荐度 |
|------|--------|----------------|--------|
| **方案 1: CMake + Kconfig** | 中等 | 100% | ⭐⭐⭐⭐⭐ 推荐 |
| **方案 2: CMake option()** | 低 | 0% | ⭐⭐⭐ 简单项目 |
| **方案 3: ccmake/cmake-gui** | 低 | 0% | ⭐⭐ 图形化配置 |
| **方案 4: Kconfiglib (Python)** | 高 | 100% | ⭐⭐⭐⭐ 高级需求 |

### 推荐方案

**方案 1: CMake + Kconfig（推荐）**

- ✅ 保留完整的 Kconfig 功能（menuconfig）
- ✅ 保留现有的 Kconfig 文件
- ✅ 保留现有的 defconfig 文件
- ✅ CMake 读取 Kconfig 生成的配置
- ✅ 实现成本低

---

## 目录

1. [方案 1: CMake + Kconfig 集成（推荐）](#方案-1-cmake--kconfig-集成推荐)
2. [方案 2: 纯 CMake option()](#方案-2-纯-cmake-option)
3. [方案 3: ccmake/cmake-gui 图形化配置](#方案-3-ccmakecmake-gui-图形化配置)
4. [方案 4: Kconfiglib (Python)](#方案-4-kconfiglib-python)
5. [实际案例参考](#实际案例参考)
6. [推荐实施方案](#推荐实施方案)

---

## 方案 1: CMake + Kconfig 集成（推荐）

### 1.1 核心思路

**保留 Kconfig，CMake 读取配置**：

```
用户配置阶段:
  make menuconfig
  ↓
  生成 .config
  ↓
  scripts/kconfig/conf --syncconfig
  ↓
  生成 include/generated/autoconf.h

CMake 配置阶段:
  cmake ..
  ↓
  读取 .config 或 autoconf.h
  ↓
  设置 CMake 变量
  ↓
  生成 Makefile/Ninja

编译阶段:
  make -j$(nproc)
```

### 1.2 实现方式

#### 方式 A: CMake 读取 .config 文件

```cmake
# CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(EMS VERSION 1.0.0 LANGUAGES C)

# 检查 .config 是否存在
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/.config")
    message(FATAL_ERROR 
        "Configuration file .config not found!\n"
        "Please run 'make menuconfig' or 'make defconfig' first."
    )
endif()

# 读取 .config 文件
file(STRINGS "${CMAKE_SOURCE_DIR}/.config" CONFIG_LINES)

# 解析配置选项
foreach(LINE ${CONFIG_LINES})
    # 跳过注释和空行
    if(LINE MATCHES "^#" OR LINE STREQUAL "")
        continue()
    endif()
    
    # 解析 CONFIG_XXX=y
    if(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=y")
        set(CONFIG_${CMAKE_MATCH_1} ON CACHE BOOL "Kconfig option" FORCE)
        message(STATUS "Enabled: CONFIG_${CMAKE_MATCH_1}")
    endif()
    
    # 解析 CONFIG_XXX=n 或 # CONFIG_XXX is not set
    if(LINE MATCHES "^# CONFIG_([A-Z0-9_]+) is not set" OR 
       LINE MATCHES "^CONFIG_([A-Z0-9_]+)=n")
        set(CONFIG_${CMAKE_MATCH_1} OFF CACHE BOOL "Kconfig option" FORCE)
    endif()
    
    # 解析 CONFIG_XXX="string"
    if(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=\"(.*)\"")
        set(CONFIG_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE STRING "Kconfig option" FORCE)
    endif()
    
    # 解析 CONFIG_XXX=123
    if(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=([0-9]+)")
        set(CONFIG_${CMAKE_MATCH_1} ${CMAKE_MATCH_2} CACHE STRING "Kconfig option" FORCE)
    endif()
endforeach()

# 使用配置
if(CONFIG_OSAL)
    add_subdirectory(core/osal)
endif()

if(CONFIG_HAL)
    add_subdirectory(core/hal)
endif()

# ... 更多模块
```

**优点**：
- ✅ 保留完整的 Kconfig 功能
- ✅ 保留 menuconfig 图形界面
- ✅ 保留 defconfig 机制
- ✅ 实现简单（~50 行 CMake 代码）

**缺点**：
- ⚠️ 需要先运行 `make menuconfig`，再运行 `cmake`
- ⚠️ 修改配置后需要重新运行 `cmake`

---

#### 方式 B: CMake 读取 autoconf.h

```cmake
# CMakeLists.txt

# 检查 autoconf.h 是否存在
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/include/generated/autoconf.h")
    message(FATAL_ERROR 
        "Configuration file autoconf.h not found!\n"
        "Please run 'make menuconfig' or 'make defconfig' first."
    )
endif()

# 读取 autoconf.h
file(STRINGS "${CMAKE_SOURCE_DIR}/include/generated/autoconf.h" AUTOCONF_LINES)

# 解析配置选项
foreach(LINE ${AUTOCONF_LINES})
    # 解析 #define CONFIG_XXX 1
    if(LINE MATCHES "^#define CONFIG_([A-Z0-9_]+) 1")
        set(CONFIG_${CMAKE_MATCH_1} ON CACHE BOOL "Kconfig option" FORCE)
    endif()
    
    # 解析 #define CONFIG_XXX "string"
    if(LINE MATCHES "^#define CONFIG_([A-Z0-9_]+) \"(.*)\"")
        set(CONFIG_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE STRING "Kconfig option" FORCE)
    endif()
    
    # 解析 #define CONFIG_XXX 123
    if(LINE MATCHES "^#define CONFIG_([A-Z0-9_]+) ([0-9]+)")
        set(CONFIG_${CMAKE_MATCH_1} ${CMAKE_MATCH_2} CACHE STRING "Kconfig option" FORCE)
    endif()
endforeach()

# 使用配置
if(CONFIG_OSAL)
    add_subdirectory(core/osal)
endif()
```

**优点**：
- ✅ 与 C 代码使用相同的配置文件
- ✅ 保证一致性

**缺点**：
- ⚠️ 需要先运行 `make syncconfig` 生成 autoconf.h

---

#### 方式 C: CMake 自定义命令调用 Kconfig

```cmake
# CMakeLists.txt

# 添加 menuconfig 目标
add_custom_target(menuconfig
    COMMAND ${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/scripts/kconfig menuconfig
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running menuconfig..."
)

# 添加 defconfig 目标
add_custom_target(defconfig
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/kconfig/conf --defconfig=configs/defconfig Kconfig
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Loading default configuration..."
)

# 自动生成 autoconf.h
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/include/generated/autoconf.h
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/kconfig/conf --syncconfig Kconfig
    DEPENDS ${CMAKE_SOURCE_DIR}/.config
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating autoconf.h..."
)

# 读取配置（同方式 B）
# ...
```

**使用方式**：

```bash
# 配置
cmake -B build
cd build
make menuconfig  # CMake 调用 Kconfig
cmake ..         # 重新配置 CMake

# 或者
make defconfig
cmake ..

# 编译
make -j$(nproc)
```

**优点**：
- ✅ 在 CMake 构建系统内调用 Kconfig
- ✅ 统一的构建流程

---

### 1.3 完整示例

```cmake
# CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(EMS VERSION 1.0.0 LANGUAGES C)

# ============================================================================
# Kconfig 集成
# ============================================================================

# 函数：读取 .config 文件
function(load_kconfig CONFIG_FILE)
    if(NOT EXISTS "${CONFIG_FILE}")
        message(FATAL_ERROR 
            "Configuration file ${CONFIG_FILE} not found!\n"
            "Please run one of the following commands first:\n"
            "  make menuconfig\n"
            "  make defconfig\n"
            "  make <board>_defconfig\n"
        )
    endif()
    
    file(STRINGS "${CONFIG_FILE}" CONFIG_LINES)
    
    foreach(LINE ${CONFIG_LINES})
        # 跳过注释和空行
        if(LINE MATCHES "^#" OR LINE STREQUAL "")
            continue()
        endif()
        
        # CONFIG_XXX=y
        if(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=y")
            set(CONFIG_${CMAKE_MATCH_1} ON PARENT_SCOPE)
            message(STATUS "  CONFIG_${CMAKE_MATCH_1}=y")
        endif()
        
        # CONFIG_XXX=n or # CONFIG_XXX is not set
        if(LINE MATCHES "^# CONFIG_([A-Z0-9_]+) is not set" OR 
           LINE MATCHES "^CONFIG_([A-Z0-9_]+)=n")
            set(CONFIG_${CMAKE_MATCH_1} OFF PARENT_SCOPE)
        endif()
        
        # CONFIG_XXX="string"
        if(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=\"(.*)\"")
            set(CONFIG_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" PARENT_SCOPE)
            message(STATUS "  CONFIG_${CMAKE_MATCH_1}=\"${CMAKE_MATCH_2}\"")
        endif()
        
        # CONFIG_XXX=123
        if(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=([0-9]+)")
            set(CONFIG_${CMAKE_MATCH_1} ${CMAKE_MATCH_2} PARENT_SCOPE)
            message(STATUS "  CONFIG_${CMAKE_MATCH_1}=${CMAKE_MATCH_2}")
        endif()
    endforeach()
endfunction()

# 加载配置
message(STATUS "Loading Kconfig configuration from .config...")
load_kconfig("${CMAKE_SOURCE_DIR}/.config")

# ============================================================================
# 编译选项
# ============================================================================

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wundef -O2)

# ============================================================================
# 子目录（根据 Kconfig 配置）
# ============================================================================

# Core 模块
if(CONFIG_OSAL)
    message(STATUS "Building OSAL")
    add_subdirectory(core/osal)
endif()

if(CONFIG_HAL)
    message(STATUS "Building HAL")
    add_subdirectory(core/hal)
endif()

if(CONFIG_PCL)
    message(STATUS "Building PCL")
    add_subdirectory(core/pcl)
endif()

if(CONFIG_PDL)
    message(STATUS "Building PDL")
    add_subdirectory(core/pdl)
endif()

if(CONFIG_ACL)
    message(STATUS "Building ACL")
    add_subdirectory(core/acl)
endif()

# Products 模块
add_subdirectory(products/ccm)

# Tests 模块
if(CONFIG_BUILD_TESTING)
    message(STATUS "Building tests")
    add_subdirectory(tests)
endif()
```

**使用流程**：

```bash
# 1. 配置 Kconfig
make menuconfig
# 或
make ccm_h200_am625_debug_defconfig

# 2. 配置 CMake
mkdir build && cd build
cmake ..

# 输出：
# -- Loading Kconfig configuration from .config...
# --   CONFIG_OSAL=y
# --   CONFIG_HAL=y
# --   CONFIG_PCL=y
# -- Building OSAL
# -- Building HAL
# -- Building PCL
# ...

# 3. 编译
make -j$(nproc)

# 4. 修改配置
cd ..
make menuconfig  # 修改配置
cd build
cmake ..         # 重新配置 CMake
make -j$(nproc)  # 重新编译
```

---

### 1.4 优化：自动检测配置变化

```cmake
# CMakeLists.txt

# 配置文件依赖
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/.config
)

# 当 .config 变化时，CMake 会自动重新配置
```

**效果**：

```bash
# 修改配置
make menuconfig

# 编译时自动重新配置
cd build
make -j$(nproc)
# -- Configuring done (configuration changed)
# -- Generating done
# -- Build files have been written to: /home/wanguo/EMS/build
# [1/150] Building C object ...
```

---

## 方案 2: 纯 CMake option()

### 2.1 实现方式

**放弃 Kconfig，使用 CMake 原生配置**：

```cmake
# CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(EMS VERSION 1.0.0 LANGUAGES C)

# ============================================================================
# 配置选项（替代 Kconfig）
# ============================================================================

# Core 模块
option(CONFIG_OSAL "Enable OSAL" ON)
option(CONFIG_HAL "Enable HAL" ON)
option(CONFIG_PCL "Enable PCL" ON)
option(CONFIG_PDL "Enable PDL" ON)
option(CONFIG_ACL "Enable ACL" ON)

# OSAL 子选项
option(CONFIG_OSAL_NETWORK "Enable OSAL network support" ON)
option(CONFIG_OSAL_IPC "Enable OSAL IPC support" ON)

# 平台选项
set(CONFIG_ARCH "x86_64" CACHE STRING "Target architecture")
set_property(CACHE CONFIG_ARCH PROPERTY STRINGS x86_64 arm arm64 riscv)

set(CONFIG_OS "linux" CACHE STRING "Target OS")
set_property(CACHE CONFIG_OS PROPERTY STRINGS linux windows rtos bare)

# Products 模块
option(CONFIG_PROJECT_H200_AM625 "Enable H200 AM625 project" ON)

# Applications
option(CONFIG_BUILD_CCM_COLLECTOR "Build CCM collector" ON)
option(CONFIG_BUILD_CCM_HEALTH "Build CCM health" ON)
option(CONFIG_BUILD_CCM_LOGGER "Build CCM logger" ON)

# Tests
option(CONFIG_BUILD_TESTING "Build tests" OFF)

# ============================================================================
# 使用配置
# ============================================================================

if(CONFIG_OSAL)
    add_subdirectory(core/osal)
endif()

if(CONFIG_HAL)
    add_subdirectory(core/hal)
endif()
```

**配置方式**：

```bash
# 命令行配置
cmake -B build \
    -DCONFIG_OSAL=ON \
    -DCONFIG_HAL=ON \
    -DCONFIG_ARCH=arm64 \
    -DCONFIG_OS=linux

# 或使用 ccmake（文本界面）
ccmake build

# 或使用 cmake-gui（图形界面）
cmake-gui
```

**优点**：
- ✅ 纯 CMake，无需额外工具
- ✅ 跨平台（Windows、macOS、Linux）
- ✅ IDE 集成好

**缺点**：
- ❌ 失去 Kconfig 的强大功能
- ❌ 失去 menuconfig 图形界面
- ❌ 失去 defconfig 机制
- ❌ 失去依赖关系管理（select、depends on）
- ❌ 需要重写所有配置

---

## 方案 3: ccmake/cmake-gui 图形化配置

### 3.1 ccmake（文本界面）

```bash
# 配置
ccmake -B build

# 界面：
┌─────────────────────────────────────────────────────────┐
│ CMAKE_BUILD_TYPE         Release                        │
│ CONFIG_OSAL              ON                             │
│ CONFIG_HAL               ON                             │
│ CONFIG_PCL               ON                             │
│ CONFIG_ARCH              arm64                          │
│ CONFIG_OS                linux                          │
│                                                         │
│ [c] Configure  [g] Generate  [q] Quit                  │
└─────────────────────────────────────────────────────────┘
```

### 3.2 cmake-gui（图形界面）

```bash
cmake-gui
```

**优点**：
- ✅ 图形化配置
- ✅ 跨平台

**缺点**：
- ❌ 不如 menuconfig 强大
- ❌ 无依赖关系管理

---

## 方案 4: Kconfiglib (Python)

### 4.1 实现方式

**使用 Python 的 Kconfig 实现**：

```python
# scripts/kconfig_to_cmake.py

import kconfiglib

# 加载 Kconfig
kconf = kconfiglib.Kconfig('Kconfig')
kconf.load_config('.config')

# 生成 CMake 配置文件
with open('build/kconfig.cmake', 'w') as f:
    for sym in kconf.unique_defined_syms:
        if sym.str_value == 'y':
            f.write(f'set(CONFIG_{sym.name} ON)\n')
        elif sym.str_value == 'n':
            f.write(f'set(CONFIG_{sym.name} OFF)\n')
        else:
            f.write(f'set(CONFIG_{sym.name} "{sym.str_value}")\n')
```

```cmake
# CMakeLists.txt

# 运行 Python 脚本生成配置
execute_process(
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/kconfig_to_cmake.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 包含生成的配置
include(${CMAKE_BINARY_DIR}/kconfig.cmake)
```

**优点**：
- ✅ 保留完整的 Kconfig 功能
- ✅ 更灵活的处理

**缺点**：
- ⚠️ 需要安装 kconfiglib
- ⚠️ 增加复杂度

---

## 实际案例参考

### 案例 1: Zephyr RTOS

**Zephyr 使用 CMake + Kconfig**：

```cmake
# Zephyr 的做法
find_package(Kconfig REQUIRED)

# 运行 Kconfig
execute_process(
    COMMAND ${KCONFIG_BINARY} --syncconfig Kconfig
)

# 读取配置
include(${CMAKE_BINARY_DIR}/kconfig.cmake)
```

### 案例 2: ESP-IDF

**ESP-IDF 使用 CMake + Kconfig**：

```cmake
# ESP-IDF 的做法
idf_build_get_property(config_dir CONFIG_DIR)
include(${config_dir}/sdkconfig.cmake)
```

### 案例 3: Buildroot

**Buildroot 使用 Make + Kconfig**：

- 保持 Make 作为主构建系统
- 使用 Kconfig 配置
- 各个包可以使用 CMake

---

## 推荐实施方案

### 方案选择

**推荐：方案 1 - CMake + Kconfig 集成**

理由：
1. ✅ 保留现有的 Kconfig 文件和 defconfig
2. ✅ 保留 menuconfig 图形界面
3. ✅ 实现成本低（~100 行 CMake 代码）
4. ✅ 团队无需学习新的配置方式
5. ✅ 符合航空航天项目的配置管理要求

### 实施步骤

**步骤 1: 创建 Kconfig 读取模块**

```bash
# 创建 cmake/Kconfig.cmake
cat > cmake/Kconfig.cmake << 'CMAKE_EOF'
# Kconfig 集成模块

function(load_kconfig CONFIG_FILE)
    if(NOT EXISTS "${CONFIG_FILE}")
        message(FATAL_ERROR "Configuration file not found: ${CONFIG_FILE}")
    endif()
    
    file(STRINGS "${CONFIG_FILE}" CONFIG_LINES)
    
    foreach(LINE ${CONFIG_LINES})
        if(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=y")
            set(CONFIG_${CMAKE_MATCH_1} ON PARENT_SCOPE)
        elseif(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=\"(.*)\"")
            set(CONFIG_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" PARENT_SCOPE)
        elseif(LINE MATCHES "^CONFIG_([A-Z0-9_]+)=([0-9]+)")
            set(CONFIG_${CMAKE_MATCH_1} ${CMAKE_MATCH_2} PARENT_SCOPE)
        endif()
    endforeach()
endfunction()
CMAKE_EOF
```

**步骤 2: 修改顶层 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(EMS VERSION 1.0.0 LANGUAGES C)

# 包含 Kconfig 模块
include(cmake/Kconfig.cmake)

# 加载配置
load_kconfig("${CMAKE_SOURCE_DIR}/.config")

# 使用配置
if(CONFIG_OSAL)
    add_subdirectory(core/osal)
endif()
```

**步骤 3: 保留 Make 包装器**

```makefile
# Makefile (简化版)

# Kconfig 目标
menuconfig defconfig %_defconfig:
	@$(MAKE) -C scripts/kconfig $@

# CMake 构建
build:
	@mkdir -p build
	@cd build && cmake .. && make -j$(nproc)

.PHONY: menuconfig defconfig build
```

**使用流程**：

```bash
# 配置
make menuconfig

# 编译
make build

# 或直接使用 CMake
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## 总结

### 答案

✅ **CMake 可以支持 Kconfig 配置方式**

### 推荐方案

**CMake + Kconfig 集成**（方案 1）

- 保留 Kconfig 的所有功能
- 保留 menuconfig 图形界面
- 保留 defconfig 机制
- CMake 读取 Kconfig 生成的配置
- 实现成本低（~100 行代码）

### 不推荐

- ❌ 完全放弃 Kconfig（方案 2）
- ❌ 使用 ccmake 替代 menuconfig（方案 3）

### 实际案例

- Zephyr RTOS 使用 CMake + Kconfig
- ESP-IDF 使用 CMake + Kconfig
- 证明这是成熟可行的方案

---

**文档版本**: 1.0  
**最后更新**: 2026-05-27  
**结论**: CMake 完全可以与 Kconfig 集成，推荐使用方案 1

