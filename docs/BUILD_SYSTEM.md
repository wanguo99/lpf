# EMS 编译系统架构

本文档说明EMS的模块化编译系统设计，该设计参考了NASA cFS（Core Flight System）的编译框架。

## 1. 设计理念

### 1.1 核心原则

参考cFS的模块化设计，EMS采用以下核心原则：

1. **接口与实现分离**：每个模块提供接口库（仅头文件）和实现库（静态库）
2. **清晰的依赖层次**：OSAL → HAL → PCL → PDL → Apps
3. **独立编译**：每个模块可独立编译和测试
4. **跨平台支持**：通过平台配置支持多种目标架构

### 1.2 与cFS的对比

| cFS | EMS | 说明 |
|-----|---------|------|
| OSAL | OSAL | 操作系统抽象层 |
| PSP | HAL | 平台/硬件支持包 |
| cFE Core | PCL + PDL | 核心服务（配置+外设驱动） |
| Apps | Apps | 应用层 |
| ut_assert | test_runner | 测试框架 |

## 2. 模块架构

### 2.1 依赖层次

```
┌─────────────────────────────────────────┐
│         应用层 (Apps)                    │
│  sample_app, ...                        │
└──────────────┬──────────────────────────┘
               │ 依赖 pdl_public_api
┌──────────────▼──────────────────────────┐
│    PDL (Peripheral Driver Layer)        │
│  MCU, Satellite, BMC 外设驱动           │
└──────────────┬──────────────────────────┘
               │ 依赖 pcl_public_api
               │      hal_public_api
┌──────────────▼──────────────────────────┐
│    PCL (Hardware Config)            │
│  硬件配置库（类似设备树）                │
└──────────────┬──────────────────────────┘
               │ 依赖 osal_public_api
┌──────────────▼──────────────────────────┐
│    HAL (Hardware Abstraction Layer)     │
│  CAN, Serial 驱动                       │
└──────────────┬──────────────────────────┘
               │ 依赖 osal_public_api
┌──────────────▼──────────────────────────┐
│    OSAL (OS Abstraction Layer)          │
│  操作系统抽象接口                        │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│      操作系统 (Linux/RTEMS/VxWorks)      │
└─────────────────────────────────────────┘
```

### 2.2 模块结构

每个模块提供两个CMake目标：

1. **接口库 (`<module>_public_api`)**
   - 类型：INTERFACE库（仅头文件）
   - 用途：供上层模块使用，提供头文件路径
   - 命名空间：`ems::<module>_public_api`

2. **实现库 (`<module>`)**
   - 类型：STATIC库（静态库）
   - 用途：包含实际代码实现
   - 命名空间：`ems::<module>`

### 2.3 依赖关系

上层模块依赖下层模块的方式：

```cmake
# PUBLIC依赖：接口库（继承头文件路径）
target_link_libraries(hal PUBLIC ems::osal_public_api)

# PRIVATE依赖：实现库（运行时链接）
target_link_libraries(hal PRIVATE ems::osal)
```

这种设计的优点：
- 编译时只需要头文件（通过接口库）
- 链接时才需要实现库
- 避免循环依赖
- 支持独立编译

## 3. 模块详解

### 3.1 OSAL层

**文件**：`osal/CMakeLists.txt`

**提供的库**：
- `osal_public_api` - 接口库
- `osal` - 实现库

**依赖**：
- 系统库：`Threads::Threads`, `rt`

**特点**：
- 唯一允许包含操作系统相关代码的层
- 提供跨平台抽象接口
- 支持POSIX/RTEMS/VxWorks等多种OS

**使用示例**：
```cmake
target_link_libraries(my_module
    PUBLIC ems::osal_public_api   # 头文件
    PRIVATE ems::osal             # 实现
)
```

### 3.2 HAL层

**文件**：`hal/CMakeLists.txt`

**提供的库**：
- `hal_public_api` - 接口库
- `hal` - 实现库

**依赖**：
- `ems::osal_public_api` (PUBLIC)
- `ems::osal` (PRIVATE)

**特点**：
- 唯一允许包含硬件平台相关代码的层
- 封装CAN、串口等硬件驱动
- 根据PLATFORM变量选择不同实现（linux/rtems）

### 3.3 PCL层

**文件**：`pcl/CMakeLists.txt`

**提供的库**：
- `pcl_public_api` - 接口库
- `pcl` - 实现库

**依赖**：
- `ems::osal_public_api` (PUBLIC)
- `ems::osal` (PRIVATE)

**特点**：
- 纯数据配置库，类似Linux设备树
- 以外设为单位管理硬件配置
- 支持多平台多产品配置

**平台配置**：
```
pcl/platform/
├── ti/am6254/H200_100P/        # TI AM6254平台
│   ├── h200_100p_base.c
│   ├── h200_100p_v1.c
│   └── h200_100p_v2.c
└── vendor_demo/                # 演示平台
    └── ...
```

### 3.4 PDL层

**文件**：`pdl/CMakeLists.txt`

**提供的库**：
- `pdl_public_api` - 接口库
- `pdl` - 实现库

**依赖**：
- `ems::osal_public_api` (PUBLIC)
- `ems::hal_public_api` (PUBLIC)
- `ems::pcl_public_api` (PUBLIC)
- `ems::osal` (PRIVATE)
- `ems::hal` (PRIVATE)
- `ems::pcl` (PRIVATE)

**特点**：
- 统一外设管理层
- 管理MCU、卫星、BMC等外设
- 通过PCL获取硬件配置

### 3.5 Apps层

**文件**：`apps/sample_app/CMakeLists.txt`

**依赖**：
- `ems::osal`（或其他需要的模块）

**特点**：
- 应用层代码
- 可以使用OSAL、HAL、PDL等所有下层接口
- 编译为可执行文件

### 3.6 Tests层

**文件**：`tests/CMakeLists.txt`

**提供的库**：
- `test_runner` - 测试运行器库

**依赖**：
- `ems::pdl`
- `ems::hal`
- `ems::osal`

**特点**：
- 统一测试入口（unit-test）
- 支持交互式菜单和命令行参数
- 可选代码覆盖率支持

## 4. 构建流程

### 4.1 顶层CMakeLists.txt

**文件**：`CMakeLists.txt`

**关键步骤**：

1. **平台配置**
```cmake
if(NOT DEFINED PLATFORM)
    set(PLATFORM "native" CACHE STRING "Target platform")
endif()
```

2. **模块化构建**
```cmake
# 按依赖顺序添加子目录
add_subdirectory(osal)
add_subdirectory(hal)
add_subdirectory(pcl)
add_subdirectory(pdl)
add_subdirectory(apps)
add_subdirectory(tests)
```

3. **创建命名空间别名**
```cmake
add_library(ems::osal_public_api ALIAS osal_public_api)
add_library(ems::osal ALIAS osal)
# ... 其他模块
```

### 4.2 构建命令

```bash
# 清理
./build.sh -c

# 编译（Release模式）
./build.sh

# 编译（Debug模式）
./build.sh -d

# 运行测试
./output/target/bin/unit-test -a

# 运行示例应用
./output/target/bin/sample_app
```

### 4.3 输出目录

```
output/
├── build/              # 编译中间文件
│   ├── lib/            # 静态库
│   │   ├── libosal.a
│   │   ├── libhal.a
│   │   ├── libpcl.a
│   │   └── libpdl.a
│   └── ...
└── target/             # 最终产物
    ├── bin/            # 可执行文件
    │   ├── sample_app
    │   └── unit-test
    └── lib/            # 静态库（复制）
```

## 5. 独立编译

### 5.1 编译单个模块

```bash
cd output/build
make osal -j$(nproc)        # 仅编译OSAL
make hal -j$(nproc)         # 仅编译HAL
make pcl -j$(nproc)     # 仅编译PCL
make pdl -j$(nproc)         # 仅编译PDL
make sample_app -j$(nproc)  # 仅编译示例应用
make unit-test -j$(nproc)   # 仅编译测试
```

### 5.2 增量编译

CMake自动处理增量编译，只重新编译修改的文件：

```bash
# 修改某个源文件后
./build.sh  # 只编译修改的文件及依赖它的模块
```

## 6. 跨平台支持

### 6.1 平台配置

通过`PLATFORM`变量选择目标平台：

```bash
# 本地编译（默认）
./build.sh

# 交叉编译（ARM Linux）
./build.sh -p generic-linux
```

### 6.2 添加新平台

1. 在顶层`CMakeLists.txt`添加平台配置：
```cmake
elseif(PLATFORM STREQUAL "rtems")
    set(CMAKE_C_COMPILER "arm-rtems5-gcc")
    set(CMAKE_SYSTEM_NAME RTEMS)
    set(CMAKE_SYSTEM_PROCESSOR arm)
```

2. 在各模块添加平台相关实现：
```cmake
# hal/CMakeLists.txt
elseif(PLATFORM STREQUAL "rtems")
    set(HAL_SOURCES
        src/rtems/hal_can.c
        src/rtems/hal_serial.c
    )
```

## 7. 安装支持

### 7.1 安装规则

每个模块都提供安装规则，支持`make install`：

```bash
cd output/build
make install DESTDIR=/path/to/install
```

### 7.2 find_package支持

每个模块生成CMake配置文件，支持`find_package`：

```cmake
# 在其他项目中使用EMS
find_package(osal REQUIRED)
find_package(hal REQUIRED)

target_link_libraries(my_app
    ems::osal
    ems::hal
)
```

## 8. 最佳实践

### 8.1 添加新模块

1. 创建模块目录和CMakeLists.txt
2. 定义接口库和实现库
3. 在顶层CMakeLists.txt添加子目录
4. 创建命名空间别名

### 8.2 模块依赖规则

- **上层模块**：PUBLIC依赖下层接口库，PRIVATE依赖下层实现库
- **同层模块**：避免相互依赖
- **下层模块**：不能依赖上层模块

### 8.3 头文件组织

- **公共接口**：放在`include/`目录，通过接口库暴露
- **内部实现**：放在`src/`目录，不暴露给外部
- **平台相关**：放在`src/<platform>/`目录

### 8.4 编译选项

- 使用`target_compile_options`而不是全局`CMAKE_C_FLAGS`
- 使用`PRIVATE`避免污染上层模块
- 关键警告使用`-Werror`确保代码质量

## 9. 常见问题

### 9.1 找不到头文件

**原因**：没有正确链接接口库

**解决**：
```cmake
# 错误
target_include_directories(my_module PRIVATE ${OSAL_INCLUDE_DIR})

# 正确
target_link_libraries(my_module PUBLIC ems::osal_public_api)
```

### 9.2 链接错误

**原因**：没有链接实现库

**解决**：
```cmake
target_link_libraries(my_module
    PUBLIC ems::osal_public_api   # 头文件
    PRIVATE ems::osal             # 实现（必须）
)
```

### 9.3 循环依赖

**原因**：模块间相互依赖

**解决**：
- 检查依赖层次，确保单向依赖
- 将共享代码提取到下层模块
- 使用接口库打破循环

## 10. 总结

EMS的编译系统参考cFS设计，具有以下优点：

1. **模块化**：清晰的层次结构，便于维护和扩展
2. **独立编译**：每个模块可独立编译和测试
3. **跨平台**：支持多种操作系统和硬件平台
4. **可重用**：通过接口库和命名空间，便于在其他项目中使用
5. **现代化**：使用CMake现代特性（INTERFACE库、命名空间别名等）

这种设计使得EMS能够适应从开发板到实际卫星等各种部署场景。
