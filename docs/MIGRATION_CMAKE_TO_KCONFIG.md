# CMake 到 Kconfig+Makefile 迁移指南

## 概述

EMS 项目已从 CMake 构建系统迁移到 Linux 内核风格的 Kconfig + Makefile 构建系统。本文档说明迁移的原因、变化和如何适应新系统。

## 迁移原因

### 为什么从 CMake 迁移？

1. **更好的配置管理**
   - Kconfig 提供菜单驱动的配置界面
   - 支持配置依赖检查（`depends on`）
   - 配置选项可视化，易于理解模块关系

2. **更符合嵌入式开发习惯**
   - Linux 内核、Buildroot、U-Boot 都使用 Kconfig
   - 嵌入式开发者更熟悉这套工具链
   - 更容易集成到 Buildroot 等构建系统

3. **更简洁的模块 Makefile**
   - 通用构建框架（类似 Buildroot package infrastructure）
   - 模块只需定义变量，无需编写编译规则
   - 减少重复代码，降低维护成本

4. **更好的条件编译支持**
   - Kconfig 配置直接生成 C 宏定义
   - Makefile 可以根据配置条件编译源文件
   - 避免编译不需要的代码

## 主要变化

### 1. 构建命令变化

| 操作 | CMake (旧) | Kconfig+Makefile (新) |
|------|-----------|---------------------|
| 配置 | `cmake -B build -DCMAKE_BUILD_TYPE=Release` | `make menuconfig` 或 `make ccm_h200_am625_release_defconfig` |
| 编译 | `cmake --build build -j$(nproc)` | `make -j$(nproc)` |
| 清理 | `rm -rf build` | `make clean` 或 `make distclean` |
| 安装 | `cmake --install build` | `make install INSTALL_PREFIX=/opt/ems` |
| 头文件安装 | N/A | `make headers_install INSTALL_PREFIX=/opt/ems` |
| Out-of-tree | `cmake -B /tmp/build` | `make O=/tmp/build` |

### 2. 目录结构变化

```
旧结构 (CMake):
├── CMakeLists.txt
├── build/                  # 构建输出
│   ├── bin/
│   └── lib/
├── cmake/
│   └── toolchains/         # 交叉编译工具链
└── core/
    └── osal/
        ├── CMakeLists.txt
        └── include/

新结构 (Kconfig+Makefile):
├── Makefile                # 顶层 Makefile
├── Kconfig                 # 顶层配置
├── .config                 # 当前配置（生成）
├── configs/                # 预设配置
│   └── ccm_h200_am625_release_defconfig
├── include/                # 顶层导出头文件
│   ├── osal/
│   ├── hal/
│   ├── generated/
│   │   └── autoconf.h      # Kconfig 生成
│   └── ems_config.h
├── bin/                    # 可执行文件输出
├── lib/                    # 库文件输出
├── scripts/
│   ├── Makefile.lib        # 通用编译规则
│   ├── Makefile.build      # 通用构建框架
│   ├── install.mk          # 安装规则
│   └── kconfig/            # Kconfig 工具
└── core/
    └── osal/
        ├── Kconfig         # OSAL 配置选项
        ├── Makefile        # OSAL 构建脚本
        └── include/
```

### 3. 配置方式变化

**CMake 方式（旧）**:
```bash
# 通过命令行参数配置
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_HAL_CAN=ON \
  -DENABLE_HAL_UART=ON
```

**Kconfig 方式（新）**:
```bash
# 交互式配置
make menuconfig

# 或使用预设配置
make ccm_h200_am625_debug_defconfig

# 配置保存在 .config 文件中
# 生成的 C 宏定义在 include/generated/autoconf.h
```

### 4. 条件编译变化

**CMake 方式（旧）**:
```cmake
# CMakeLists.txt
if(ENABLE_HAL_CAN)
  list(APPEND SRCS hal_can.c)
endif()
```

```c
// C 代码
#ifdef ENABLE_HAL_CAN
  // CAN 相关代码
#endif
```

**Kconfig 方式（新）**:
```kconfig
# Kconfig
config HAL_CAN
    bool "CAN bus driver"
    depends on HAL
    default y
```

```makefile
# Makefile
SRCS-$(CONFIG_HAL_CAN) += hal_can.c
```

```c
// C 代码
#include <generated/autoconf.h>
#ifdef CONFIG_HAL_CAN
  // CAN 相关代码
#endif
```

### 5. 模块 Makefile 简化

**CMake 方式（旧）**:
```cmake
# core/osal/CMakeLists.txt (约 50 行)
cmake_minimum_required(VERSION 3.16)

set(OSAL_SRCS
    src/posix/lib/osal_errno.c
    src/posix/ipc/osal_mutex.c
    # ... 更多源文件
)

add_library(osal SHARED ${OSAL_SRCS})
target_include_directories(osal PUBLIC include)
target_link_libraries(osal pthread rt)

install(TARGETS osal LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/osal)
```

**Kconfig+Makefile 方式（新）**:
```makefile
# core/osal/Makefile (约 20 行)
MODULE_NAME := osal
MODULE_TYPE := shared_lib
SRC_DIR := $(srctree)/core/$(MODULE_NAME)/src/posix

SRCS := \
    lib/osal_errno.c \
    ipc/osal_mutex.c

EXPORT_HEADERS := \
    osal.h \
    osal_types.h \
    ipc/osal_mutex.h

LDLIBS := -lpthread -lrt

include $(srctree)/scripts/Makefile.build
```

## 迁移步骤

### 对于开发者

1. **删除旧的构建目录**
   ```bash
   rm -rf build/
   ```

2. **配置新构建系统**
   ```bash
   make menuconfig
   # 或
   make ccm_h200_am625_release_defconfig
   ```

3. **编译**
   ```bash
   make -j$(nproc)
   ```

4. **更新脚本和文档**
   - 将脚本中的 `cmake` 命令替换为 `make`
   - 更新 CI/CD 配置

### 对于模块开发者

1. **删除 CMakeLists.txt**
   ```bash
   rm core/mymodule/CMakeLists.txt
   ```

2. **创建 Kconfig**
   ```bash
   cat > core/mymodule/Kconfig << 'EOF'
   config MYMODULE
       bool "My Module"
       depends on OSAL
       default y
       help
         Description of my module.
   EOF
   ```

3. **创建简化的 Makefile**
   ```bash
   cat > core/mymodule/Makefile << 'EOF'
   MODULE_NAME := mymodule
   MODULE_TYPE := shared_lib
   
   SRCS := \
       mymodule.c \
       mymodule_utils.c
   
   EXPORT_HEADERS := \
       mymodule.h
   
   LDLIBS := -L$(objtree)/lib -losal
   
   include $(srctree)/scripts/Makefile.build
   EOF
   ```

4. **更新顶层配置**
   - 在 `Kconfig` 中添加 `source "core/mymodule/Kconfig"`
   - 在 `core/Makefile` 中添加模块到 `MODULES` 列表

## 新功能

### 1. 头文件导出系统

```bash
# 导出模块头文件到顶层 include/
make -C core/osal headers_install

# 安装头文件到系统
make headers_install INSTALL_PREFIX=/usr/local
```

### 2. Out-of-tree 构建

```bash
# 在独立目录构建，保持源码树干净
make O=/tmp/ems-build defconfig
make O=/tmp/ems-build -j$(nproc)
```

### 3. 配置依赖检查

Kconfig 自动检查模块依赖关系：
```kconfig
config PDL
    bool "PDL"
    depends on OSAL && HAL && PCL  # 自动检查依赖
    default y
```

### 4. 条件编译优化

只编译启用的功能：
```makefile
SRCS-$(CONFIG_HAL_CAN) += hal_can.c
SRCS-$(CONFIG_HAL_UART) += hal_serial.c
SRCS-$(CONFIG_HAL_I2C) += hal_i2c.c
```

## 常见问题

### Q: 为什么不能用 `cmake` 命令了？

A: 项目已完全迁移到 Makefile 构建系统，不再使用 CMake。请使用 `make` 命令。

### Q: 如何设置 Debug/Release 模式？

A: 使用 Kconfig 配置：
```bash
make menuconfig
# 进入 "Build Options" -> 选择 "Debug build"
```
或使用预设配置：
```bash
make ccm_h200_am625_debug_defconfig    # Debug
make ccm_h200_am625_release_defconfig  # Release
```

### Q: 如何交叉编译？

A: 在 Kconfig 中配置目标架构：
```bash
make menuconfig
# 进入 "Build Options" -> "Target Architecture"
```

### Q: 旧的 build/ 目录怎么办？

A: 可以安全删除：
```bash
rm -rf build/
```
新系统输出到 `bin/` 和 `lib/` 目录。

### Q: 如何查看所有配置选项？

A: 运行 `make menuconfig` 浏览所有选项，或查看 `Kconfig` 文件。

### Q: 如何保存自定义配置？

A: 使用 `savedefconfig`：
```bash
make savedefconfig
# 保存到 defconfig 文件
# 可以复制到 configs/ 目录作为预设
cp defconfig configs/my_custom_defconfig
```

## 优势总结

1. **更直观的配置**: 菜单式配置界面，一目了然
2. **依赖检查**: Kconfig 自动检查模块依赖，避免配置错误
3. **更简洁**: 模块 Makefile 从 50+ 行减少到 20 行左右
4. **更灵活**: 支持条件编译，只编译需要的代码
5. **更标准**: 符合 Linux 内核和嵌入式开发习惯
6. **更易维护**: 通用构建框架，减少重复代码
7. **更好的集成**: 易于集成到 Buildroot 等构建系统

## 参考资料

- [docs/BUILD_SYSTEM.md](BUILD_SYSTEM.md) - 构建系统详细文档
- [Linux Kernel Kbuild](https://www.kernel.org/doc/html/latest/kbuild/)
- [Buildroot Manual](https://buildroot.org/downloads/manual/manual.html)
