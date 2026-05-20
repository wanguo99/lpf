# EMS 编译框架快速参考

## 通用 Makefile 框架

### 1. 构建库 (Makefile.lib)

```makefile
LIB_NAME := mylib
LIB_TYPE := shared    # shared/static/both
SRCS := file1.c file2.c
EXPORT_HEADERS := mylib.h
LDLIBS := -losal
include $(srctree)/scripts/Makefile.lib
```

**命令**：
```bash
make -C core/mylib                    # 构建
make -C core/mylib headers_install    # 导出头文件
make -C core/mylib clean              # 清理
```

### 2. 构建应用 (Makefile.app)

```makefile
APP_NAME := myapp
SRCS := main.c logic.c
LDLIBS := -losal -lhal
include $(srctree)/scripts/Makefile.app
```

**命令**：
```bash
make -C products/myapp                # 构建
make -C products/myapp install        # 安装
```

### 3. 构建内核模块 (Makefile.ko)

```makefile
KO_NAME := mydriver
SRCS := main.c ops.c
include $(srctree)/scripts/Makefile.ko
```

**命令**：
```bash
make -C drivers/mydriver              # 构建
sudo make -C drivers/mydriver load    # 加载
sudo make -C drivers/mydriver unload  # 卸载
```

### 4. 构建 Windows DLL (Makefile.dll)

```makefile
DLL_NAME := mylib
SRCS := mylib.c win32.c
EXPORT_HEADERS := mylib.h
include $(srctree)/scripts/Makefile.dll
```

**命令**：
```bash
export CROSS_COMPILE=x86_64-w64-mingw32-
make -C core/mylib -f Makefile.windows
```

## 条件编译

```makefile
# 基于 Kconfig 配置
SRCS-$(CONFIG_FEATURE_A) += feature_a.c
SRCS-$(CONFIG_FEATURE_B) += feature_b.c
```

## 顶层命令

```bash
# 配置
make menuconfig                       # 交互式配置
make ccm_h200_am625_release_defconfig # 加载预设

# 编译
make -j$(nproc)                       # 编译所有
make core -j$(nproc)                  # 仅编译核心模块
make products -j$(nproc)              # 仅编译产品

# 安装
make install INSTALL_PREFIX=/opt/ems          # 安装库和二进制
make headers_install INSTALL_PREFIX=/opt/ems  # 安装头文件
make install-all INSTALL_PREFIX=/opt/ems      # 完整安装

# 清理
make clean                            # 清理编译产物
make distclean                        # 清理编译和配置
make mrproper                         # 完全清理

# Out-of-tree 构建
make O=/tmp/build defconfig
make O=/tmp/build -j$(nproc)
```

## 头文件结构

```
core/mylib/include/     → 模块内部（开发时）
include/mylib/          → 顶层导出（跨模块引用）
/usr/local/include/     → 系统安装（外部应用）
```

**引用方式**：
```c
#include "internal.h"           // 模块内部
#include <mylib/mylib.h>        // 跨模块
#include <generated/autoconf.h> // Kconfig 配置
```

## Kconfig 依赖

```kconfig
config MYMODULE
    bool "My Module"
    depends on OSAL && HAL    # 声明依赖
    default y
```

## 框架文件

| 文件 | 用途 |
|------|------|
| `Makefile.rules` | 通用编译规则 |
| `Makefile.lib` | 库构建（.so/.a） |
| `Makefile.app` | 应用构建 |
| `Makefile.ko` | 内核模块构建 |
| `Makefile.dll` | Windows DLL 构建 |

## 添加新构建类型

```bash
# 1. 复制现有框架
cp scripts/Makefile.lib scripts/Makefile.dylib

# 2. 修改构建规则
# 编辑 Makefile.dylib

# 3. 在模块中使用
include $(srctree)/scripts/Makefile.dylib
```

## 详细文档

- [docs/BUILD_SYSTEM.md](docs/BUILD_SYSTEM.md) - 构建系统详细文档
- [docs/MAKEFILE_FRAMEWORK.md](docs/MAKEFILE_FRAMEWORK.md) - 框架使用指南
- [BUILD_FRAMEWORK_SUMMARY.md](BUILD_FRAMEWORK_SUMMARY.md) - 优化总结
- [CLAUDE.md](CLAUDE.md) - 开发指南
