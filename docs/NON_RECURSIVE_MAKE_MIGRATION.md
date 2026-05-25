# EMS 非递归 Make 完整迁移方案

> **文档版本**: 1.0  
> **创建日期**: 2026-05-25  
> **作者**: EMS Team  
> **状态**: 规划中

---

## 📋 目录

- [一、方案概述](#一方案概述)
- [二、架构设计](#二架构设计)
- [三、完整实现代码](#三完整实现代码)
- [四、自动化工具](#四自动化工具)
- [五、迁移实施计划](#五迁移实施计划)
- [六、风险管理](#六风险管理)
- [七、验证清单](#七验证清单)
- [八、后续优化](#八后续优化)
- [九、总结](#九总结)
- [附录](#附录)

---


## 一、方案概述

### 1.1 目标

将 EMS 项目从递归 Make迁移到完全非递归 Make，为后期大规模扩展做好准备。

### 1.2 预期收益

| 指标 | 当前（递归） | 目标（非递归） | 提升 |
| --- | --- | --- | --- |
| 全量编译时间 | 6秒 | 4秒 | 33% |
| 增量编译时间 | 2秒 | 0.5秒 | 75% |
| 并行度（-j8） | 3-4核 | 7-8核 | 2倍 |
| 文件数量扩展到1000+ | 60秒 | 25秒 | 58% |
| 文件数量扩展到5000+ | 5分钟 | 2分钟 | 60% |

### 1.3 迁移策略

渐进式迁移（4个阶段，共6周）：
1. 阶段1：基础设施搭建（1周）
2. 阶段2：Core 层迁移（2周）
3. 阶段3：Products 层迁移（2周）
4. 阶段4：优化和文档（1周）

---
## 二、架构设计

### 2.1 目录结构（保持不变）

```
EMS/
```

|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |
|  |

### 2.2 核心文件说明

| 文件 | 作用 | 维护者 |
| --- | --- | --- |
| Makefile | 顶层构建入口，包含所有 module.mk | 架构师 |
| scripts/rules.mk | 通用编译规则（.c→.o, .o→.so, .o→bin） | 架构师 |
| scripts/functions.mk | 辅助函数（路径转换、依赖生成等） | 架构师 |
| */module.mk | 模块规则（源文件、编译标志、依赖） | 模块开发者 |
| scripts/auto-module.sh | 自动生成 module.mk 的脚本 | 架构师 |

---
## 三、完整实现代码

### 3.1 顶层 Makefile

```makefile
# =============================================================================
# EMS 非递归 Makefile
# 版本：1.0
# 作者：EMS Team
# 日期：2026-05-25
# =============================================================================

# -----------------------------------------------------------------------------
# 基本配置
# -----------------------------------------------------------------------------

# 项目根目录
export srctree := $(CURDIR)
export objtree := $(CURDIR)

# 输出目录
export BIN_DIR := $(objtree)/bin
export LIB_DIR := $(objtree)/lib
export OBJ_DIR := $(objtree)/.obj
export DEP_DIR := $(objtree)/.deps

# 静默输出
ifeq ($(V),1)
Q :=
else
Q := @
endif

# -----------------------------------------------------------------------------
# 工具链配置
# -----------------------------------------------------------------------------

# 编译器
CC      := $(CROSS_COMPILE)gcc
CXX     := $(CROSS_COMPILE)g++
LD      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
STRIP   := $(CROSS_COMPILE)strip

# 基础编译标志
CFLAGS  := -Wall -Wextra -O2 -fPIC
CFLAGS  += -I$(srctree)/include

# 链接标志
LDFLAGS := -L$(LIB_DIR)

# 调试/发布模式
ifeq ($(DEBUG),1)
CFLAGS += -g -O0 -DDEBUG
else
CFLAGS += -O2 -DNDEBUG
endif

# -----------------------------------------------------------------------------
# 全局变量
# -----------------------------------------------------------------------------

# 所有目标（由 module.mk 填充）
ALL_TARGETS :=

# 所有清理文件（由 module.mk 填充）
CLEAN_FILES :=

# 所有依赖文件
DEP_FILES :=

# -----------------------------------------------------------------------------
# 包含辅助脚本
# -----------------------------------------------------------------------------

include scripts/functions.mk
include scripts/rules.mk

# -----------------------------------------------------------------------------
# 包含所有模块规则
# -----------------------------------------------------------------------------

# Core 层
include core/osal/module.mk
include core/hal/module.mk
include core/pcl/module.mk
include core/pdl/module.mk
include core/acl/module.mk

# Products 层 - 库
include products/ccm/libs/libccm/module.mk
include products/ccm/h200_am625/module.mk

# Products 层 - 应用
include products/ccm/apps/ccm_collector/module.mk
include products/ccm/apps/ccm_logger/module.mk
include products/ccm/apps/ccm_health/module.mk
include products/ccm/apps/ccm_supervisor/module.mk
include products/ccm/apps/ccm_comm/module.mk

# -----------------------------------------------------------------------------
# 默认目标
# -----------------------------------------------------------------------------

.PHONY: all
all: $(ALL_TARGETS)
    @echo ""
    @echo "=== Build Complete ==="
    @echo "Libraries: $(LIB_DIR)"
    @echo "Binaries:  $(BIN_DIR)"
    @echo ""

# -----------------------------------------------------------------------------
# 清理规则
# -----------------------------------------------------------------------------

.PHONY: clean
clean:
    @echo "  CLEAN   $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)"
    $(Q)rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR) $(DEP_DIR)
    $(Q)rm -f $(CLEAN_FILES)

.PHONY: mrproper
mrproper: clean
    @echo "  CLEAN   .config"
    $(Q)rm -f .config .config.old

# -----------------------------------------------------------------------------
# 帮助信息
# -----------------------------------------------------------------------------

.PHONY: help
help:
    @echo "EMS 非递归 Make 构建系统"
    @echo ""
    @echo "常用目标："
    @echo "  all          - 构建所有目标（默认）"
    @echo "  clean        - 清理构建产物"
    @echo "  mrproper     - 清理所有（包括配置）"
    @echo "  help         - 显示此帮助信息"
    @echo ""
    @echo "单独构建："
    @echo "  lib/libosal.so           - 构建 OSAL 库"
    @echo "  lib/libhal.so            - 构建 HAL 库"
    @echo "  bin/ccm_collector        - 构建 ccm_collector 应用"
    @echo ""
    @echo "选项："
    @echo "  V=1          - 显示详细编译命令"
    @echo "  DEBUG=1      - 调试模式（-g -O0）"
    @echo "  -j<N>        - 并行编译（N 个任务）"
    @echo ""
    @echo "示例："
    @echo "  make -j8              - 8 线程并行编译"
    @echo "  make V=1              - 显示详细命令"
    @echo "  make DEBUG=1          - 调试模式编译"
    @echo "  make lib/libosal.so   - 只编译 OSAL 库"
    @echo ""

# -----------------------------------------------------------------------------
# 依赖文件
# -----------------------------------------------------------------------------

# 包含所有自动生成的依赖文件
-include $(wildcard $(DEP_DIR)/**/*.d)

# -----------------------------------------------------------------------------
# 调试信息
# -----------------------------------------------------------------------------

.PHONY: debug-vars
debug-vars:
    @echo "=== Build Variables ==="
    @echo "srctree:     $(srctree)"
    @echo "objtree:     $(objtree)"
    @echo "CC:          $(CC)"
    @echo "CFLAGS:      $(CFLAGS)"
    @echo "LDFLAGS:     $(LDFLAGS)"
    @echo ""
    @echo "=== Targets ==="
    @echo "ALL_TARGETS: $(ALL_TARGETS)"
    @echo ""

.PHONY: debug-deps
debug-deps:
    @echo "=== Dependency Graph ==="
    @for target in $(ALL_TARGETS); do \
            echo "$$target:"; \
            $(MAKE) -n $$target 2>&1 | grep "Prerequisite" | sed 's/^/  /'; \
    done

```

### 3.2 scripts/rules.mk

```makefile
# =============================================================================
# 通用编译规则
# =============================================================================

# -----------------------------------------------------------------------------
# 编译 .c → .o
# -----------------------------------------------------------------------------

$(OBJ_DIR)/%.o: %.c
    @mkdir -p $(dir $@)
    @mkdir -p $(dir $(DEP_DIR)/$*.d)
    @echo "  CC      $<"
    $(Q)$(CC) $(CFLAGS) $(CFLAGS_$<) -MMD -MP -MF $(DEP_DIR)/$*.d -c -o $@ $<

# 说明：
# -MMD: 生成依赖文件（不包含系统头文件）
# -MP:  为每个头文件生成伪目标（避免删除头文件后编译失败）
# -MF:  指定依赖文件路径

# -----------------------------------------------------------------------------
# 编译 .cpp → .o
# -----------------------------------------------------------------------------

$(OBJ_DIR)/%.o: %.cpp
    @mkdir -p $(dir $@)
    @mkdir -p $(dir $(DEP_DIR)/$*.d)
    @echo "  CXX     $<"
    $(Q)$(CXX) $(CXXFLAGS) $(CXXFLAGS_$<) -MMD -MP -MF $(DEP_DIR)/$*.d -c -o $@ $<

# -----------------------------------------------------------------------------
# 链接动态库 .o → .so
# -----------------------------------------------------------------------------

# 通用规则（由 module.mk 中的具体目标触发）
$(LIB_DIR)/%.so:
    @mkdir -p $(dir $@)
    @echo "  LD      $@"
    $(Q)$(LD) -shared -Wl,-soname,$(notdir $@).1 -o $@ $(filter %.o,$^) $(LDFLAGS_$@)
    @echo "  STRIP   $@"
    $(Q)$(STRIP) --strip-unneeded $@

# 说明：
# -shared:              生成动态库
# -Wl,-soname:          设置 SONAME
# $(filter %.o,$^):     只链接 .o 文件，排除 .so 依赖
# $(LDFLAGS_$@):        目标特定的链接标志（由 module.mk 定义）

# -----------------------------------------------------------------------------
# 链接静态库 .o → .a
# -----------------------------------------------------------------------------

$(LIB_DIR)/%.a:
    @mkdir -p $(dir $@)
    @echo "  AR      $@"
    $(Q)$(AR) rcs $@ $(filter %.o,$^)

# -----------------------------------------------------------------------------
# 链接可执行文件 .o → bin
# -----------------------------------------------------------------------------

$(BIN_DIR)/%:
    @mkdir -p $(dir $@)
    @echo "  LD      $@"
    $(Q)$(CC) -o $@ $(filter %.o,$^) $(LDFLAGS_$@)
    @echo "  STRIP   $@"
    $(Q)$(STRIP) $@

# 说明：
# $(filter %.o,$^):     只链接 .o 文件，排除 .so 依赖
# $(LDFLAGS_$@):        目标特定的链接标志（由 module.mk 定义）

# -----------------------------------------------------------------------------
# 伪目标
# -----------------------------------------------------------------------------

.PHONY: FORCE
FORCE:

# 用于强制重新构建某些目标

```

### 3.3 scripts/functions.mk

```makefile
# =============================================================================
# 辅助函数
# =============================================================================

# -----------------------------------------------------------------------------
# 路径转换函数
# -----------------------------------------------------------------------------

# 将源文件路径转换为目标文件路径
# 用法：$(call src-to-obj,core/osal/src/osal_thread.c)
# 结果：.obj/core/osal/src/osal_thread.o
src-to-obj = $(patsubst %.c,$(OBJ_DIR)/%.o,$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(1)))

# 将源文件路径转换为依赖文件路径
# 用法：$(call src-to-dep,core/osal/src/osal_thread.c)
# 结果：.deps/core/osal/src/osal_thread.d
src-to-dep = $(patsubst %.c,$(DEP_DIR)/%.d,$(patsubst %.cpp,$(DEP_DIR)/%.d,$(1)))

# -----------------------------------------------------------------------------
# 模块定义函数
# -----------------------------------------------------------------------------

# 定义一个动态库模块
# 用法：$(call define-shared-lib,osal,$(osal_SRCS),$(osal_DEPS))
# 参数：
#   $(1) - 模块名（如 osal）
#   $(2) - 源文件列表
#   $(3) - 依赖的库列表（如 $(LIB_DIR)/libhal.so）
define define-shared-lib
$(1)_OBJS := $(call src-to-obj,$(2))
$(LIB_DIR)/lib$(1).so: $$($(1)_OBJS) $(3)
ALL_TARGETS += $(LIB_DIR)/lib$(1).so
CLEAN_FILES += $$($(1)_OBJS)
endef

# 定义一个静态库模块
# 用法：$(call define-static-lib,osal,$(osal_SRCS))
define define-static-lib
$(1)_OBJS := $(call src-to-obj,$(2))
$(LIB_DIR)/lib$(1).a: $$($(1)_OBJS)
ALL_TARGETS += $(LIB_DIR)/lib$(1).a
CLEAN_FILES += $$($(1)_OBJS)
endef

# 定义一个可执行文件模块
# 用法：$(call define-executable,ccm_collector,$(ccm_collector_SRCS),$(ccm_collector_DEPS))
define define-executable
$(1)_OBJS := $(call src-to-obj,$(2))
$(BIN_DIR)/$(1): $$($(1)_OBJS) $(3)
ALL_TARGETS += $(BIN_DIR)/$(1)
CLEAN_FILES += $$($(1)_OBJS)
endef

# -----------------------------------------------------------------------------
# 依赖声明函数
# -----------------------------------------------------------------------------

# 为目标文件添加编译标志
# 用法：$(call add-cflags,$(osal_OBJS),-I$(srctree)/include/osal)
add-cflags = $(foreach obj,$(1),$(eval CFLAGS_$(obj) += $(2)))

# 为目标添加链接标志
# 用法：$(call add-ldflags,$(LIB_DIR)/libosal.so,-lpthread -lrt)
add-ldflags = $(eval LDFLAGS_$(1) += $(2))

# -----------------------------------------------------------------------------
# 调试函数
# -----------------------------------------------------------------------------

# 打印变量值
# 用法：$(call print-var,CFLAGS)
print-var = $(info $(1) = $($(1)))

```

### 3.4 core/osal/module.mk

```makefile
# =============================================================================
# OSAL 模块规则
# =============================================================================

# 模块名
MODULE := osal

# 模块根目录
MODULE_DIR := core/osal

# -----------------------------------------------------------------------------
# 源文件
# -----------------------------------------------------------------------------

osal_SRCS := \
    src/posix/lib/osal_errno.c \
    src/posix/lib/osal_heap.c \
    src/posix/lib/osal_stdio.c \
    src/posix/lib/osal_string.c \
    src/posix/util/osal_log.c \
    src/posix/util/osal_version.c \
    src/posix/ipc/osal_atomic.c \
    src/posix/ipc/osal_cond.c \
    src/posix/ipc/osal_mutex.c \
    src/posix/ipc/osal_semaphore.c \
    src/posix/ipc/osal_shm.c \
    src/posix/sys/osal_clock.c \
    src/posix/sys/osal_env.c \
    src/posix/sys/osal_file.c \
    src/posix/sys/osal_process.c \
    src/posix/sys/osal_sched.c \
    src/posix/sys/osal_select.c \
    src/posix/sys/osal_thread.c \
    src/posix/sys/osal_time.c \
    src/posix/sys/osal_signal.c \
    src/posix/net/osal_socket.c \
    src/posix/net/osal_termios.c

# 转换为绝对路径
osal_SRCS := $(addprefix $(MODULE_DIR)/,$(osal_SRCS))

# 生成目标文件列表
osal_OBJS := $(call src-to-obj,$(osal_SRCS))

# -----------------------------------------------------------------------------
# 编译标志
# -----------------------------------------------------------------------------

# 为所有目标文件添加头文件路径
$(osal_OBJS): CFLAGS += -I$(srctree)/include/osal
$(osal_OBJS): CFLAGS += -I$(srctree)/include/osal/ipc
$(osal_OBJS): CFLAGS += -I$(srctree)/include/osal/lib
$(osal_OBJS): CFLAGS += -I$(srctree)/include/osal/net
$(osal_OBJS): CFLAGS += -I$(srctree)/include/osal/sys
$(osal_OBJS): CFLAGS += -I$(srctree)/include/osal/util

# -----------------------------------------------------------------------------
# 链接标志
# -----------------------------------------------------------------------------

LDFLAGS_$(LIB_DIR)/libosal.so := -lpthread -lrt

# -----------------------------------------------------------------------------
# 目标：动态库
# -----------------------------------------------------------------------------

$(LIB_DIR)/libosal.so: $(osal_OBJS)

# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += $(LIB_DIR)/libosal.so
CLEAN_FILES += $(osal_OBJS)

```

### 3.5 core/hal/module.mk

```makefile
# =============================================================================
# HAL 模块规则
# =============================================================================

MODULE := hal
MODULE_DIR := core/hal

# -----------------------------------------------------------------------------
# 源文件
# -----------------------------------------------------------------------------

hal_SRCS := \
    src/generic-linux/hal_can_linux.c \
    src/generic-linux/hal_serial_linux.c \
    src/generic-linux/hal_i2c_linux.c \
    src/generic-linux/hal_spi_linux.c \
    src/generic-linux/hal_gpio_linux.c \
    src/generic-linux/hal_watchdog_linux.c

hal_SRCS := $(addprefix $(MODULE_DIR)/,$(hal_SRCS))
hal_OBJS := $(call src-to-obj,$(hal_SRCS))

# -----------------------------------------------------------------------------
# 编译标志
# -----------------------------------------------------------------------------

$(hal_OBJS): CFLAGS += -I$(srctree)/include/hal
$(hal_OBJS): CFLAGS += -I$(srctree)/include/hal/config
$(hal_OBJS): CFLAGS += -I$(srctree)/include/osal

# -----------------------------------------------------------------------------
# 链接标志
# -----------------------------------------------------------------------------

# 关键：添加 --no-as-needed，强制记录依赖
LDFLAGS_$(LIB_DIR)/libhal.so := -Wl,--no-as-needed -L$(LIB_DIR) -losal

# -----------------------------------------------------------------------------
# 目标：动态库
# -----------------------------------------------------------------------------

# 关键：显式声明依赖 libosal.so
$(LIB_DIR)/libhal.so: $(hal_OBJS) $(LIB_DIR)/libosal.so

# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += $(LIB_DIR)/libhal.so
CLEAN_FILES += $(hal_OBJS)

```

### 3.6 core/pcl/module.mk

```makefile
# =============================================================================
# PCL 模块规则
# =============================================================================

MODULE := pcl
MODULE_DIR := core/pcl

# -----------------------------------------------------------------------------
# 源文件
# -----------------------------------------------------------------------------

pcl_SRCS := \
    src/pcl_api.c \
    src/pcl_can.c

pcl_SRCS := $(addprefix $(MODULE_DIR)/,$(pcl_SRCS))
pcl_OBJS := $(call src-to-obj,$(pcl_SRCS))

# -----------------------------------------------------------------------------
# 编译标志
# -----------------------------------------------------------------------------

$(pcl_OBJS): CFLAGS += -I$(srctree)/include/pcl
$(pcl_OBJS): CFLAGS += -I$(srctree)/include/pcl/api
$(pcl_OBJS): CFLAGS += -I$(srctree)/include/hal
$(pcl_OBJS): CFLAGS += -I$(srctree)/include/osal

# -----------------------------------------------------------------------------
# 链接标志
# -----------------------------------------------------------------------------

LDFLAGS_$(LIB_DIR)/libpcl.so := -Wl,--no-as-needed -L$(LIB_DIR) -lhal -losal

# -----------------------------------------------------------------------------
# 目标：动态库
# -----------------------------------------------------------------------------

$(LIB_DIR)/libpcl.so: $(pcl_OBJS) $(LIB_DIR)/libhal.so $(LIB_DIR)/libosal.so

# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += $(LIB_DIR)/libpcl.so
CLEAN_FILES += $(pcl_OBJS)

```

### 3.7 core/pdl/module.mk

```makefile
# =============================================================================
# PDL 模块规则
# =============================================================================

MODULE := pdl
MODULE_DIR := core/pdl

# -----------------------------------------------------------------------------
# 源文件
# -----------------------------------------------------------------------------

pdl_SRCS := \
    src/can/pdl_can.c \
    src/can/pdl_can_db.c \
    src/can/pdl_can_msg.c \
    src/watchdog/pdl_watchdog.c \
    src/serial/pdl_serial.c

pdl_SRCS := $(addprefix $(MODULE_DIR)/,$(pdl_SRCS))
pdl_OBJS := $(call src-to-obj,$(pdl_SRCS))

# -----------------------------------------------------------------------------
# 编译标志
# -----------------------------------------------------------------------------

$(pdl_OBJS): CFLAGS += -I$(srctree)/include/pdl
$(pdl_OBJS): CFLAGS += -I$(srctree)/include/pcl
$(pdl_OBJS): CFLAGS += -I$(srctree)/include/hal
$(pdl_OBJS): CFLAGS += -I$(srctree)/include/osal

# -----------------------------------------------------------------------------
# 链接标志
# -----------------------------------------------------------------------------

LDFLAGS_$(LIB_DIR)/libpdl.so := -Wl,--no-as-needed -L$(LIB_DIR) -lpcl -lhal -losal

# -----------------------------------------------------------------------------
# 目标：动态库
# -----------------------------------------------------------------------------

$(LIB_DIR)/libpdl.so: $(pdl_OBJS) $(LIB_DIR)/libpcl.so $(LIB_DIR)/libhal.so $(LIB_DIR)/libosal.so

# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += $(LIB_DIR)/libpdl.so
CLEAN_FILES += $(pdl_OBJS)

```

### 3.8 core/acl/module.mk

```makefile
# =============================================================================
# ACL 模块规则
# =============================================================================

MODULE := acl
MODULE_DIR := core/acl

# -----------------------------------------------------------------------------
# 源文件
# -----------------------------------------------------------------------------

acl_SRCS := \
    src/acl_api.c \
    src/acl_config.c

acl_SRCS := $(addprefix $(MODULE_DIR)/,$(acl_SRCS))
acl_OBJS := $(call src-to-obj,$(acl_SRCS))

# -----------------------------------------------------------------------------
# 编译标志
# -----------------------------------------------------------------------------

$(acl_OBJS): CFLAGS += -I$(srctree)/include/acl
$(acl_OBJS): CFLAGS += -I$(srctree)/include/pdl
$(acl_OBJS): CFLAGS += -I$(srctree)/include/pcl
$(acl_OBJS): CFLAGS += -I$(srctree)/include/hal
$(acl_OBJS): CFLAGS += -I$(srctree)/include/osal

# -----------------------------------------------------------------------------
# 链接标志
# -----------------------------------------------------------------------------

LDFLAGS_$(LIB_DIR)/libacl.so := -Wl,--no-as-needed -L$(LIB_DIR) -lpdl -lpcl -lhal -losal

# -----------------------------------------------------------------------------
# 目标：动态库
# -----------------------------------------------------------------------------

$(LIB_DIR)/libacl.so: $(acl_OBJS) $(LIB_DIR)/libpdl.so $(LIB_DIR)/libpcl.so $(LIB_DIR)/libhal.so $(LIB_DIR)/libosal.so

# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += $(LIB_DIR)/libacl.so
CLEAN_FILES += $(acl_OBJS)

```

### 3.9 products/ccm/apps/ccm_collector/module.mk

```makefile
# =============================================================================
# ccm_collector 应用规则
# =============================================================================

MODULE := ccm_collector
MODULE_DIR := products/ccm/apps/ccm_collector

# -----------------------------------------------------------------------------
# 源文件
# -----------------------------------------------------------------------------

ccm_collector_SRCS := \
    src/main.c \
    src/ccm_collector.c

ccm_collector_SRCS := $(addprefix $(MODULE_DIR)/,$(ccm_collector_SRCS))
ccm_collector_OBJS := $(call src-to-obj,$(ccm_collector_SRCS))

# -----------------------------------------------------------------------------
# 编译标志
# -----------------------------------------------------------------------------

$(ccm_collector_OBJS): CFLAGS += -I$(srctree)/$(MODULE_DIR)/include
$(ccm_collector_OBJS): CFLAGS += -I$(srctree)/include/acl
$(ccm_collector_OBJS): CFLAGS += -I$(srctree)/include/osal

# -----------------------------------------------------------------------------
# 链接标志
# -----------------------------------------------------------------------------

LDFLAGS_$(BIN_DIR)/ccm_collector := \
    -L$(LIB_DIR) \
    -Wl,-rpath,$$ORIGIN/../lib \
    -lh200_am625 -lccm -lacl -lpdl -lpcl -lhal -losal
    
# -----------------------------------------------------------------------------
# 目标：可执行文件
# -----------------------------------------------------------------------------

$(BIN_DIR)/ccm_collector: $(ccm_collector_OBJS) \
    $(LIB_DIR)/libh200_am625.so \
    $(LIB_DIR)/libccm.so \
    $(LIB_DIR)/libacl.so \
    $(LIB_DIR)/libpdl.so \
    $(LIB_DIR)/libpcl.so \
    $(LIB_DIR)/libhal.so \
    $(LIB_DIR)/libosal.so
    
# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += $(BIN_DIR)/ccm_collector
CLEAN_FILES += $(ccm_collector_OBJS)

---
```

## 四、自动化工具

### 4.1 scripts/auto-module.sh

```bash
#!/bin/bash
# =============================================================================
# 自动生成 module.mk 的脚本
#

✻ Worked for 2m 27s

❯ 继续                                                                                                                      

● # 用法：./scripts/auto-module.sh <module_dir> <module_type>
# 示例：./scripts/auto-module.sh core/osal shared_lib
# =============================================================================

set -e

# -----------------------------------------------------------------------------
# 参数检查
# -----------------------------------------------------------------------------

if [ $# -lt 2 ]; then
  echo "用法: $0 <module_dir> <module_type>"
  echo ""
  echo "module_type 可选值："
  echo "  shared_lib    - 动态库"
  echo "  static_lib    - 静态库"
  echo "  executable    - 可执行文件"
  echo ""
  echo "示例："
  echo "  $0 core/osal shared_lib"
  echo "  $0 products/ccm/apps/ccm_collector executable"
  exit 1
fi

MODULE_DIR="$1"
MODULE_TYPE="$2"

# 检查目录是否存在
if [ ! -d "$MODULE_DIR" ]; then
  echo "错误：目录 $MODULE_DIR 不存在"
  exit 1
fi

# 提取模块名
MODULE_NAME=$(basename "$MODULE_DIR")

# 输出文件
OUTPUT_FILE="$MODULE_DIR/module.mk"

# -----------------------------------------------------------------------------
# 查找源文件
# -----------------------------------------------------------------------------

echo "正在扫描 $MODULE_DIR 中的源文件..."

# 查找所有 .c 文件（相对于模块目录）
SRCS=$(find "$MODULE_DIR" -name "*.c" -type f | sed "s|^$MODULE_DIR/||" | sort)

if [ -z "$SRCS" ]; then
  echo "警告：未找到任何 .c 文件"
fi

# 统计文件数量
NUM_SRCS=$(echo "$SRCS" | wc -l)
echo "找到 $NUM_SRCS 个源文件"

# -----------------------------------------------------------------------------
# 生成 module.mk
# -----------------------------------------------------------------------------

echo "正在生成 $OUTPUT_FILE..."

cat > "$OUTPUT_FILE" << EOF
# =============================================================================
# $MODULE_NAME 模块规则
# 自动生成于: $(date '+%Y-%m-%d %H:%M:%S')
# =============================================================================

MODULE := $MODULE_NAME
MODULE_DIR := $MODULE_DIR

# -----------------------------------------------------------------------------
# 源文件
# -----------------------------------------------------------------------------

${MODULE_NAME}_SRCS := \\
EOF

# 写入源文件列表
echo "$SRCS" | while read -r src; do
  if [ -n "$src" ]; then
      echo "        $src \\" >> "$OUTPUT_FILE"
  fi
done

# 移除最后一个反斜杠
sed -i '$ s/ \\$//' "$OUTPUT_FILE"

cat >> "$OUTPUT_FILE" << EOF

${MODULE_NAME}_SRCS := \$(addprefix \$(MODULE_DIR)/,\$(${MODULE_NAME}_SRCS))
${MODULE_NAME}_OBJS := \$(call src-to-obj,\$(${MODULE_NAME}_SRCS))

# -----------------------------------------------------------------------------
# 编译标志
# -----------------------------------------------------------------------------

# TODO: 添加模块特定的编译标志
# 示例：
# \$(${MODULE_NAME}_OBJS): CFLAGS += -I\$(srctree)/include/$MODULE_NAME
# \$(${MODULE_NAME}_OBJS): CFLAGS += -I\$(srctree)/include/osal

# -----------------------------------------------------------------------------
# 链接标志
# -----------------------------------------------------------------------------

EOF

# 根据模块类型生成不同的链接标志
case "$MODULE_TYPE" in
  shared_lib)
      cat >> "$OUTPUT_FILE" << EOF
# TODO: 添加依赖的库
# 示例：
# LDFLAGS_\$(LIB_DIR)/lib${MODULE_NAME}.so := -Wl,--no-as-needed -L\$(LIB_DIR) -losal

# -----------------------------------------------------------------------------
# 目标：动态库
# -----------------------------------------------------------------------------

# TODO: 添加依赖的库文件
# 示例：
# \$(LIB_DIR)/lib${MODULE_NAME}.so: \$(${MODULE_NAME}_OBJS) \$(LIB_DIR)/libosal.so
\$(LIB_DIR)/lib${MODULE_NAME}.so: \$(${MODULE_NAME}_OBJS)

# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += \$(LIB_DIR)/lib${MODULE_NAME}.so
CLEAN_FILES += \$(${MODULE_NAME}_OBJS)
EOF
      ;;

  static_lib)
      cat >> "$OUTPUT_FILE" << EOF
# 静态库不需要链接标志

# -----------------------------------------------------------------------------
# 目标：静态库
# -----------------------------------------------------------------------------

\$(LIB_DIR)/lib${MODULE_NAME}.a: \$(${MODULE_NAME}_OBJS)

# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += \$(LIB_DIR)/lib${MODULE_NAME}.a
CLEAN_FILES += \$(${MODULE_NAME}_OBJS)
EOF
      ;;

  executable)
      cat >> "$OUTPUT_FILE" << EOF
# TODO: 添加依赖的库
# 示例：
# LDFLAGS_\$(BIN_DIR)/${MODULE_NAME} := \\
#     -L\$(LIB_DIR) \\
#     -Wl,-rpath,\$\$ORIGIN/../lib \\
#     -losal -lpthread

# -----------------------------------------------------------------------------
# 目标：可执行文件
# -----------------------------------------------------------------------------

# TODO: 添加依赖的库文件
# 示例：
# \$(BIN_DIR)/${MODULE_NAME}: \$(${MODULE_NAME}_OBJS) \$(LIB_DIR)/libosal.so
\$(BIN_DIR)/${MODULE_NAME}: \$(${MODULE_NAME}_OBJS)

# -----------------------------------------------------------------------------
# 注册到全局
# -----------------------------------------------------------------------------

ALL_TARGETS += \$(BIN_DIR)/${MODULE_NAME}
CLEAN_FILES += \$(${MODULE_NAME}_OBJS)
EOF
      ;;

  *)
      echo "错误：未知的模块类型 $MODULE_TYPE"
      rm -f "$OUTPUT_FILE"
      exit 1
      ;;
esac

echo ""
echo "=== 生成完成 ==="
echo "文件: $OUTPUT_FILE"
echo ""
echo "下一步："
echo "1. 编辑 $OUTPUT_FILE，填写 TODO 部分"
echo "2. 在顶层 Makefile 中添加: include $MODULE_DIR/module.mk"
echo "3. 运行 make 测试"
echo ""

```

### 4.2 scripts/check-deps.sh

```bash
#!/bin/bash
# =============================================================================
# 检查动态库依赖关系的脚本
# =============================================================================

set -e

LIB_DIR="${1:-.staging/lib}"

if [ ! -d "$LIB_DIR" ]; then
  echo "错误：目录 $LIB_DIR 不存在"
  exit 1
fi

echo "=== 检查动态库依赖关系 ==="
echo "目录: $LIB_DIR"
echo ""

# 检查每个 .so 文件
for lib in "$LIB_DIR"/*.so; do
  if [ ! -f "$lib" ]; then
      continue
  fi

  lib_name=$(basename "$lib")
  echo "[$lib_name]"

  # 使用 readelf 检查 NEEDED 条目
  needed=$(readelf -d "$lib" 2>/dev/null | grep NEEDED | awk '{print $5}' | tr -d '[]')

  if [ -z "$needed" ]; then
      echo "  无依赖"
  else
      echo "$needed" | while read -r dep; do
          echo "  → $dep"
      done
  fi

  echo ""
done

echo "=== 依赖关系图（文本版）==="
echo ""

# 生成依赖关系图
for lib in "$LIB_DIR"/*.so; do
  if [ ! -f "$lib" ]; then
      continue
  fi

  lib_name=$(basename "$lib" .so)
  needed=$(readelf -d "$lib" 2>/dev/null | grep NEEDED | awk '{print $5}' | tr -d '[]' | sed 's/\.so.*$//')

  if [ -z "$needed" ]; then
      echo "$lib_name"
  else
      deps=$(echo "$needed" | tr '\n' ',' | sed 's/,$//')
      echo "$lib_name → $deps"
  fi
done

echo ""
echo "=== 检查完成 ==="

```

### 4.3 scripts/benchmark.sh

```bash
#!/bin/bash
# =============================================================================
# 性能基准测试脚本
# 对比递归 Make 和非递归 Make 的性能
# =============================================================================

set -e

# 测试次数
NUM_TESTS=5

# 结果文件
RESULT_FILE="benchmark_results.txt"

echo "=== EMS 构建系统性能基准测试 ===" | tee "$RESULT_FILE"
echo "日期: $(date)" | tee -a "$RESULT_FILE"
echo "CPU: $(nproc) 核" | tee -a "$RESULT_FILE"
echo "" | tee -a "$RESULT_FILE"

# -----------------------------------------------------------------------------
# 测试递归 Make
# -----------------------------------------------------------------------------

if [ -f "Makefile.recursive" ]; then
  echo "=== 测试递归 Make ===" | tee -a "$RESULT_FILE"

  # 备份当前 Makefile
  if [ -f "Makefile" ]; then
      mv Makefile Makefile.nonrecursive
  fi
  cp Makefile.recursive Makefile

  # 全量编译测试
  echo "全量编译测试 (递归 Make):" | tee -a "$RESULT_FILE"
  total_time=0
  for i in $(seq 1 $NUM_TESTS); do
      make clean > /dev/null 2>&1
      start=$(date +%s.%N)
      make -j$(nproc) > /dev/null 2>&1
      end=$(date +%s.%N)
      elapsed=$(echo "$end - $start" | bc)
      echo "  第 $i 次: ${elapsed}s" | tee -a "$RESULT_FILE"
      total_time=$(echo "$total_time + $elapsed" | bc)
  done
  avg_time=$(echo "scale=2; $total_time / $NUM_TESTS" | bc)
  echo "  平均: ${avg_time}s" | tee -a "$RESULT_FILE"
  RECURSIVE_FULL=$avg_time

  # 增量编译测试
  echo "增量编译测试 (递归 Make):" | tee -a "$RESULT_FILE"
  total_time=0
  for i in $(seq 1 $NUM_TESTS); do
      # 修改一个文件
      touch core/osal/src/posix/lib/osal_thread.c
      start=$(date +%s.%N)
      make -j$(nproc) > /dev/null 2>&1
      end=$(date +%s.%N)
      elapsed=$(echo "$end - $start" | bc)
      echo "  第 $i 次: ${elapsed}s" | tee -a "$RESULT_FILE"
      total_time=$(echo "$total_time + $elapsed" | bc)
  done
  avg_time=$(echo "scale=2; $total_time / $NUM_TESTS" | bc)
  echo "  平均: ${avg_time}s" | tee -a "$RESULT_FILE"
  RECURSIVE_INCR=$avg_time

  echo "" | tee -a "$RESULT_FILE"

  # 恢复 Makefile
  rm -f Makefile
  if [ -f "Makefile.nonrecursive" ]; then
      mv Makefile.nonrecursive Makefile
  fi
fi

# -----------------------------------------------------------------------------
# 测试非递归 Make
# -----------------------------------------------------------------------------

echo "=== 测试非递归 Make ===" | tee -a "$RESULT_FILE"

# 全量编译测试
echo "全量编译测试 (非递归 Make):" | tee -a "$RESULT_FILE"
total_time=0
for i in $(seq 1 $NUM_TESTS); do
  make clean > /dev/null 2>&1
  start=$(date +%s.%N)
  make -j$(nproc) > /dev/null 2>&1
  end=$(date +%s.%N)
  elapsed=$(echo "$end - $start" | bc)
  echo "  第 $i 次: ${elapsed}s" | tee -a "$RESULT_FILE"
  total_time=$(echo "$total_time + $elapsed" | bc)
done
avg_time=$(echo "scale=2; $total_time / $NUM_TESTS" | bc)
echo "  平均: ${avg_time}s" | tee -a "$RESULT_FILE"
NONRECURSIVE_FULL=$avg_time

# 增量编译测试
echo "增量编译测试 (非递归 Make):" | tee -a "$RESULT_FILE"
total_time=0
for i in $(seq 1 $NUM_TESTS); do
  # 修改一个文件
  touch core/osal/src/posix/lib/osal_thread.c
  start=$(date +%s.%N)
  make -j$(nproc) > /dev/null 2>&1
  end=$(date +%s.%N)
  elapsed=$(echo "$end - $start" | bc)
  echo "  第 $i 次: ${elapsed}s" | tee -a "$RESULT_FILE"
  total_time=$(echo "$total_time + $elapsed" | bc)
done
avg_time=$(echo "scale=2; $total_time / $NUM_TESTS" | bc)
echo "  平均: ${avg_time}s" | tee -a "$RESULT_FILE"
NONRECURSIVE_INCR=$avg_time

echo "" | tee -a "$RESULT_FILE"

# -----------------------------------------------------------------------------
# 对比结果
# -----------------------------------------------------------------------------

if [ -n "$RECURSIVE_FULL" ]; then
  echo "=== 性能对比 ===" | tee -a "$RESULT_FILE"
  echo "" | tee -a "$RESULT_FILE"

  # 全量编译提升
  improvement=$(echo "scale=2; ($RECURSIVE_FULL - $NONRECURSIVE_FULL) / $RECURSIVE_FULL * 100" | bc)
  echo "全量编译:" | tee -a "$RESULT_FILE"
  echo "  递归 Make:    ${RECURSIVE_FULL}s" | tee -a "$RESULT_FILE"
  echo "  非递归 Make:  ${NONRECURSIVE_FULL}s" | tee -a "$RESULT_FILE"
  echo "  提升:         ${improvement}%" | tee -a "$RESULT_FILE"
  echo "" | tee -a "$RESULT_FILE"

  # 增量编译提升
  improvement=$(echo "scale=2; ($RECURSIVE_INCR - $NONRECURSIVE_INCR) / $RECURSIVE_INCR * 100" | bc)
  echo "增量编译:" | tee -a "$RESULT_FILE"
  echo "  递归 Make:    ${RECURSIVE_INCR}s" | tee -a "$RESULT_FILE"
  echo "  非递归 Make:  ${NONRECURSIVE_INCR}s" | tee -a "$RESULT_FILE"
  echo "  提升:         ${improvement}%" | tee -a "$RESULT_FILE"
  echo "" | tee -a "$RESULT_FILE"
fi

echo "=== 测试完成 ===" | tee -a "$RESULT_FILE"
echo "结果已保存到: $RESULT_FILE"

---
```

## 五、迁移实施计划

### 5.1 阶段1：基础设施搭建（第1周）

目标

- 搭建非递归 Make 框架
- 验证可行性

任务清单

Day 1-2：准备工作
# 1. 备份当前构建系统
cp Makefile Makefile.recursive
git add Makefile.recursive
git commit -m "备份：保存递归 Make 构建系统"

# 2. 创建分支
git checkout -b feature/non-recursive-make

# 3. 创建目录结构
mkdir -p scripts
mkdir -p docs

Day 3-4：创建基础文件
# 1. 创建顶层 Makefile（空框架）
# 2. 创建 scripts/rules.mk
# 3. 创建 scripts/functions.mk
# 4. 创建 scripts/auto-module.sh
chmod +x scripts/auto-module.sh

Day 5：试点模块（osal）
# 1. 生成 core/osal/module.mk
./scripts/auto-module.sh core/osal shared_lib

# 2. 手动编辑 module.mk，填写依赖关系

# 3. 在顶层 Makefile 中包含
echo "include core/osal/module.mk" >> Makefile

# 4. 测试编译
make clean
make lib/libosal.so -j8

# 5. 验证结果
ls -lh lib/libosal.so
readelf -d lib/libosal.so | grep NEEDED

Day 6-7：验证和调试
# 1. 对比递归 Make 和非递归 Make 的结果
make -f Makefile.recursive clean
make -f Makefile.recursive lib/libosal.so
md5sum .staging/lib/libosal.so > /tmp/recursive.md5

make clean
make lib/libosal.so
md5sum lib/libosal.so > /tmp/nonrecursive.md5

diff /tmp/recursive.md5 /tmp/nonrecursive.md5

# 2. 性能测试
time make -f Makefile.recursive clean && make -f Makefile.recursive lib/libosal.so -j8
time make clean && make lib/libosal.so -j8

# 3. 提交代码
git add .
git commit -m "阶段1：完成基础设施搭建和 osal 模块迁移"

### 5.2 阶段2：Core 层迁移（第2-3周）

目标

- 迁移所有 Core 模块
- 验证依赖关系

任务清单

Week 2：迁移 hal, pcl
# Day 1-2: hal 模块
./scripts/auto-module.sh core/hal shared_lib
# 编辑 core/hal/module.mk
# 添加依赖: libosal.so
# 测试: make lib/libhal.so

# Day 3-4: pcl 模块
./scripts/auto-module.sh core/pcl shared_lib
# 编辑 core/pcl/module.mk
# 添加依赖: libhal.so, libosal.so
# 测试: make lib/libpcl.so

# Day 5: 验证依赖关系
./scripts/check-deps.sh lib

Week 3：迁移 pdl, acl
# Day 1-2: pdl 模块
./scripts/auto-module.sh core/pdl shared_lib
# 编辑 core/pdl/module.mk
# 添加依赖: libpcl.so, libhal.so, libosal.so
# 测试: make lib/libpdl.so

# Day 3-4: acl 模块
./scripts/auto-module.sh core/acl shared_lib
# 编辑 core/acl/module.mk
# 添加依赖: libpdl.so, libpcl.so, libhal.so, libosal.so
# 测试: make lib/libacl.so

# Day 5: 全面测试
make clean
make -j8
./scripts/check-deps.sh lib

# 提交代码
git add .
git commit -m "阶段2：完成 Core 层迁移"

### 5.3 阶段3：Products 层迁移（第4-5周）

目标

- 迁移所有 Products 模块
- 完成整个构建系统迁移

任务清单

Week 4：迁移库
# Day 1-2: libccm
./scripts/auto-module.sh products/ccm/libs/libccm shared_lib
# 测试: make lib/libccm.so

# Day 3-4: libh200_am625
./scripts/auto-module.sh products/ccm/h200_am625 shared_lib
# 测试: make lib/libh200_am625.so

# Day 5: 验证
make clean
make -j8
./scripts/check-deps.sh lib

Week 5：迁移应用
# Day 1: ccm_collector
./scripts/auto-module.sh products/ccm/apps/ccm_collector executable
# 测试: make bin/ccm_collector

# Day 2: ccm_logger, ccm_health
./scripts/auto-module.sh products/ccm/apps/ccm_logger executable
./scripts/auto-module.sh products/ccm/apps/ccm_health executable

# Day 3: ccm_supervisor, ccm_comm
./scripts/auto-module.sh products/ccm/apps/ccm_supervisor executable
./scripts/auto-module.sh products/ccm/apps/ccm_comm executable

# Day 4: 全面测试
make clean
make -j8
# 测试所有应用
for app in bin/*; do
  echo "测试: $app"
  ldd "$app"
done

# Day 5: 提交代码
git add .
git commit -m "阶段3：完成 Products 层迁移"

### 5.4 阶段4：优化和文档（第6周）

目标

- 性能优化
- 编写文档
- 团队培训

任务清单

Day 1-2：性能测试
# 1. 运行基准测试
./scripts/benchmark.sh

# 2. 分析结果
cat benchmark_results.txt

# 3. 优化（如果需要）
# - 调整并行度
# - 优化依赖关系
# - 减少不必要的重新编译

Day 3-4：编写文档
# 创建以下文档：

1. docs/NON_RECURSIVE_MAKE.md
 - 非递归 Make 原理
 - 使用指南
 - 常见问题
 
2. docs/MODULE_MK_GUIDE.md
 - module.mk 编写规范
 - 示例代码
 - 最佳实践
 
3. docs/MIGRATION_GUIDE.md
 - 迁移步骤
 - 注意事项
 - 回退方案

4. 更新 README.md
 - 构建系统说明
 - 快速开始

Day 5：团队培训
1. 准备培训材料
2. 组织团队会议
3. 演示新构建系统
4. 回答问题
5. 收集反馈

Day 6-7：清理和发布
# 1. 删除旧的递归 Makefile（可选）
# git rm core/*/Makefile
# git rm products/*/Makefile

# 2. 更新 .gitignore
echo ".obj/" >> .gitignore
echo ".deps/" >> .gitignore

# 3. 最终测试
make mrproper
make ccm_h200_am625_debug_defconfig
make -j$(nproc)

# 4. 合并到主分支
git checkout master
git merge feature/non-recursive-make

# 5. 打标签
git tag -a v2.0-nonrecursive -m "迁移到非递归 Make 构建系统"
git push origin master --tags

---
## 六、风险管理

### 6.1 潜在风险

| 风险 | 影响 | 概率 | 缓解措施 |
| --- | --- | --- | --- |
| 依赖关系错误 | 高 | 中 | 充分测试，使用 check-deps.sh |
| 编译结果不一致 | 高 | 低 | 对比 md5，验证功能 |
| 团队学习曲线 | 中 | 高 | 详细文档，培训 |
| 性能提升不明显 | 低 | 中 | 基准测试，持续优化 |
| 回退困难 | 中 | 低 | 保留 Makefile.recursive |

### 6.2 回退方案

如果迁移失败，可以快速回退：

# 方案1：使用备份的递归 Makefile
mv Makefile Makefile.nonrecursive
mv Makefile.recursive Makefile
make clean
make -j8

# 方案2：切换到旧分支
git checkout master
make clean
make -j8

# 方案3：使用 git revert
git revert <commit-hash>

---
## 七、验证清单

### 7.1 功能验证

- 所有库都能正确编译
- 所有应用都能正确编译
- 动态库依赖关系正确（使用 readelf 检查）
- 应用能正常运行
- 所有 defconfig 都能编译通过

### 7.2 性能验证

- 全量编译时间 < 递归 Make
- 增量编译时间 < 递归 Make
- 并行度 > 递归 Make
- Make 启动时间 < 递归 Make

### 7.3 质量验证

- 代码风格一致
- 文档完整
- 无编译警告
- 通过静态分析
- 通过压力测试（20次并行编译）

---
## 八、后续优化

### 8.1 短期优化（1-3个月）

1. 自动依赖生成
- 使用 -MMD -MP 自动生成 .d 文件
- 减少手动维护依赖关系
2. 增量链接
- 只重新链接修改的库
- 减少链接时间
3. 并行度优化
- 分析依赖图
- 识别瓶颈
- 优化依赖关系

### 8.2 中期优化（3-6个月）

1. 分布式编译
- 集成 distcc 或 ccache
- 利用多台机器并行编译
2. 预编译头文件
- 识别常用头文件
- 生成预编译头文件
- 减少编译时间
3. 模块化配置
- 支持按需编译
- 减少不必要的模块

### 8.3 长期优化（6-12个月）

1. 迁移到 Ninja
- 使用 CMake 或 GN 生成 Ninja 文件
- 进一步提升编译速度
2. 持续集成优化
- 增量 CI 构建
- 缓存构建产物
- 减少 CI 时间
3. 构建系统监控
- 收集编译时间数据
- 分析性能瓶颈
- 持续优化

---
## 九、总结

### 9.1 关键要点

1. 渐进式迁移：分4个阶段，降低风险
2. 充分测试：每个阶段都要验证
3. 保留回退：备份递归 Makefile
4. 团队协作：文档、培训、反馈

### 9.2 预期收益

- 编译速度：提升 30-60%
- 并行度：充分利用多核 CPU
- 可维护性：依赖关系清晰
- 可扩展性：支持大规模项目

### 9.3 成功标准

- 所有功能正常
- 性能提升明显
- 团队接受度高
- 文档完整

---
附录A：module.mk 模板

A.1 动态库模板

```makefile
# =============================================================================

MODULE := <module_name>
MODULE_DIR := <module_dir>

# 源文件
<module_name>_SRCS := \
    src/file1.c \
    src/file2.c

<module_name>_SRCS := $(addprefix $(MODULE_DIR)/,$(<module_name>_SRCS))
<module_name>_OBJS := $(call src-to-obj,$(<module_name>_SRCS))

# 编译标志
$(<module_name>_OBJS): CFLAGS += -I$(srctree)/include/<module_name>

# 链接标志
LDFLAGS_$(LIB_DIR)/lib<module_name>.so := -Wl,--no-as-needed -L$(LIB_DIR) -ldep1 -ldep2

# 目标
$(LIB_DIR)/lib<module_name>.so: $(<module_name>_OBJS) $(LIB_DIR)/libdep1.so $(LIB_DIR)/libdep2.so

# 注册
ALL_TARGETS += $(LIB_DIR)/lib<module_name>.so
CLEAN_FILES += $(<module_name>_OBJS)

A.2 可执行文件模板

# =============================================================================
# <APP_NAME> 应用规则
# =============================================================================

MODULE := <app_name>
MODULE_DIR := <app_dir>

# 源文件
<app_name>_SRCS := \
    src/main.c

✻ Worked for 1m 35s

❯ 继续                                                                                                                      

● <app_name>_SRCS := $(addprefix $(MODULE_DIR)/,$(<app_name>_SRCS))
<app_name>_OBJS := $(call src-to-obj,$(<app_name>_SRCS))

# 编译标志
$(<app_name>_OBJS): CFLAGS += -I$(srctree)/$(MODULE_DIR)/include
$(<app_name>_OBJS): CFLAGS += -I$(srctree)/include/osal

# 链接标志
LDFLAGS_$(BIN_DIR)/<app_name> := \
    -L$(LIB_DIR) \
    -Wl,-rpath,$$ORIGIN/../lib \
    -ldep1 -ldep2 -losal

# 目标
$(BIN_DIR)/<app_name>: $(<app_name>_OBJS) \
    $(LIB_DIR)/libdep1.so \
    $(LIB_DIR)/libdep2.so \
    $(LIB_DIR)/libosal.so

# 注册
ALL_TARGETS += $(BIN_DIR)/<app_name>
CLEAN_FILES += $(<app_name>_OBJS)

---
附录B：常见问题解答

B.1 编译相关

Q1: 如何只编译某个模块？
# 编译单个库
make lib/libosal.so

# 编译单个应用
make bin/ccm_collector

# 编译多个目标
make lib/libosal.so lib/libhal.so

Q2: 如何查看详细的编译命令？
make V=1

Q3: 如何调试模式编译？
make DEBUG=1

Q4: 增量编译不工作？
# 检查依赖文件是否生成
ls -la .deps/

# 如果没有，检查编译命令是否包含 -MMD -MP
make V=1 | grep MMD

# 强制重新生成依赖
make clean
make -j8

Q5: 并行编译出错？
# 检查依赖关系是否正确
make debug-deps

# 串行编译测试
make -j1

# 如果串行成功，说明依赖关系有问题

B.2 依赖相关

Q6: 如何添加新的依赖库？
# 在 module.mk 中：

# 1. 添加到链接标志
LDFLAGS_$(LIB_DIR)/libmylib.so += -lnewdep

# 2. 添加到目标依赖
$(LIB_DIR)/libmylib.so: $(mylib_OBJS) $(LIB_DIR)/libnewdep.so

Q7: 如何检查动态库依赖？
# 使用脚本
./scripts/check-deps.sh lib

# 或手动检查
readelf -d lib/libacl.so | grep NEEDED
ldd lib/libacl.so

Q8: 循环依赖怎么办？
循环依赖是设计问题，需要重构代码：

1. 识别循环：A → B → C → A
2. 分析原因：哪些符号导致循环
3. 重构方案：
 - 提取公共部分到新模块
 - 使用接口/回调打破循环
 - 合并模块
 
B.3 性能相关

Q9: 编译速度没有提升？
# 1. 检查并行度
make -j8  # 使用 8 个核心

# 2. 检查依赖关系是否过于串行
make debug-deps

# 3. 使用 ccache 加速
export CC="ccache gcc"
make clean
make -j8

# 4. 运行基准测试
./scripts/benchmark.sh

Q10: 如何进一步优化？
1. 使用预编译头文件
2. 启用 ccache
3. 使用分布式编译（distcc）
4. 优化依赖关系（减少不必要的依赖）
5. 考虑迁移到 Ninja

B.4 维护相关

Q11: 如何添加新模块？
# 1. 创建模块目录
mkdir -p products/new_module/src

# 2. 添加源文件
# ... 编写代码 ...

# 3. 生成 module.mk
./scripts/auto-module.sh products/new_module shared_lib

# 4. 编辑 module.mk，填写依赖关系
vim products/new_module/module.mk

# 5. 在顶层 Makefile 中包含
echo "include products/new_module/module.mk" >> Makefile

# 6. 测试
make lib/libnew_module.so

Q12: 如何删除模块？
# 1. 从顶层 Makefile 中移除 include 行
vim Makefile
# 删除: include products/old_module/module.mk

# 2. 删除 module.mk
rm products/old_module/module.mk

# 3. 清理构建产物
make clean

Q13: 如何重命名模块？
# 1. 重命名目录
mv products/old_name products/new_name

# 2. 更新 module.mk
vim products/new_name/module.mk
# 修改 MODULE 和 MODULE_DIR

# 3. 更新顶层 Makefile
vim Makefile
# 修改 include 路径

# 4. 更新依赖该模块的其他 module.mk
grep -r "old_name" */module.mk
# 批量替换

# 5. 测试
make clean
make -j8

---
附录C：性能优化技巧

C.1 编译器优化

# 1. 启用 LTO（链接时优化）
CFLAGS += -flto
LDFLAGS += -flto

# 2. 启用优化级别
CFLAGS += -O3  # 最高优化（可能增加编译时间）
CFLAGS += -O2  # 平衡优化（推荐）

# 3. 针对特定 CPU 优化
CFLAGS += -march=native  # 本地编译
CFLAGS += -march=armv8-a # 交叉编译

# 4. 启用并行链接（gold linker）
LDFLAGS += -fuse-ld=gold
LDFLAGS += -Wl,--threads

C.2 ccache 集成

# 在 Makefile 中：
CC := ccache $(CROSS_COMPILE)gcc
CXX := ccache $(CROSS_COMPILE)g++

# 配置 ccache
export CCACHE_DIR=/path/to/ccache
export CCACHE_MAXSIZE=10G
export CCACHE_COMPRESS=1

# 安装 ccache
sudo apt-get install ccache

# 查看统计
ccache -s

# 清理缓存
ccache -C

C.3 预编译头文件

# 1. 创建预编译头文件
PCH := include/common.h.gch

$(PCH): include/common.h
    @echo "  PCH     $<"
    $(Q)$(CC) $(CFLAGS) -x c-header -o $@ $<

# 2. 使用预编译头文件
$(OBJ_DIR)/%.o: %.c $(PCH)
    @echo "  CC      $<"
    $(Q)$(CC) $(CFLAGS) -include include/common.h -c -o $@ $<

# 3. 清理
clean: clean-pch

.PHONY: clean-pch
clean-pch:
    rm -f $(PCH)

C.4 分布式编译（distcc）

# 在 Makefile 中：
CC := distcc $(CROSS_COMPILE)gcc
CXX := distcc $(CROSS_COMPILE)g++

# 增加并行度
MAKEFLAGS += -j32  # distcc 可以使用更多并行任务

# 配置 distcc
export DISTCC_HOSTS="localhost/4 server1/8 server2/8"

# 查看统计
distccmon-text 1

C.5 Ninja 集成

# 1. 生成 Ninja 文件
make -Bnk | ninja-build-gen > build.ninja

# 2. 使用 Ninja 编译
ninja -j8

# 3. 性能对比
time make -j8
time ninja -j8

---
附录D：团队协作指南

D.1 代码审查清单

module.mk 审查要点：

- 模块名和目录名一致
- 源文件列表完整
- 编译标志正确（头文件路径）
- 链接标志正确（依赖库）
- 依赖关系完整（目标依赖）
- 使用 --no-as-needed（动态库）
- 注册到 ALL_TARGETS
- 注册到 CLEAN_FILES

示例审查：

# ❌ 错误示例
$(LIB_DIR)/libhal.so: $(hal_OBJS)
# 问题：缺少依赖 libosal.so

# ✅ 正确示例
$(LIB_DIR)/libhal.so: $(hal_OBJS) $(LIB_DIR)/libosal.so

D.2 提交规范

提交信息格式：

<类型>: <简短描述>

<详细描述>

<影响范围>

类型：
- feat: 新功能
- fix: 修复
- refactor: 重构
- perf: 性能优化
- docs: 文档
- build: 构建系统

示例：

feat: 添加 libfoo 模块

- 创建 products/foo/module.mk
- 实现 foo_init() 和 foo_cleanup()
- 添加单元测试

影响范围：
- 新增库：lib/libfoo.so
- 依赖：libosal.so

D.3 分支策略

master (主分支，稳定版本)
|  |
|  |
|  |
|  |

工作流程：

# 1. 创建功能分支
git checkout develop
git checkout -b feature/module-foo

# 2. 开发和提交
# ... 编写代码 ...
git add .
git commit -m "feat: 添加 foo 模块"

# 3. 推送到远程
git push origin feature/module-foo

# 4. 创建 Pull Request
# ... 在 GitHub/GitLab 上创建 PR ...

# 5. 代码审查
# ... 团队成员审查 ...

# 6. 合并到 develop
git checkout develop
git merge feature/module-foo

# 7. 删除功能分支
git branch -d feature/module-foo

D.4 持续集成配置

.gitlab-ci.yml 示例：

stages:
- build
- test
- deploy

variables:
GIT_SUBMODULE_STRATEGY: recursive

# 构建任务
build:x86_64:
stage: build
script:
  - make mrproper
  - make x86_64_full_defconfig
  - make -j$(nproc)
artifacts:
  paths:
    - bin/
    - lib/
  expire_in: 1 day

build:arm64:
stage: build
script:
  - export CROSS_COMPILE=aarch64-linux-gnu-
  - make mrproper
  - make ccm_h200_am625_debug_defconfig
  - make -j$(nproc)
artifacts:
  paths:
    - bin/
    - lib/
  expire_in: 1 day

# 测试任务
test:unit:
stage: test
dependencies:
  - build:x86_64
script:
  - ./scripts/run-tests.sh

test:integration:
stage: test
dependencies:
  - build:x86_64
script:
  - ./scripts/run-integration-tests.sh

# 性能测试
test:performance:
stage: test
dependencies:
  - build:x86_64
script:
  - ./scripts/benchmark.sh
artifacts:
  reports:
    performance: benchmark_results.txt

# 部署任务
deploy:staging:
stage: deploy
dependencies:
  - build:arm64
script:
  - scp -r bin/ lib/ user@staging-server:/opt/ems/
only:
  - develop

deploy:production:
stage: deploy
dependencies:
  - build:arm64
script:
  - scp -r bin/ lib/ user@prod-server:/opt/ems/
only:
  - master
when: manual

---
附录E：故障排查指南

E.1 常见错误

错误1：找不到头文件

错误信息：
fatal error: osal.h: No such file or directory

解决方案：
# 检查 module.mk 中的编译标志
$(mymodule_OBJS): CFLAGS += -I$(srctree)/include/osal

---
错误2：找不到库文件

错误信息：
/usr/bin/ld: cannot find -losal

解决方案：
# 1. 检查链接标志
LDFLAGS_$(BIN_DIR)/myapp := -L$(LIB_DIR) -losal

# 2. 检查依赖关系
$(BIN_DIR)/myapp: $(myapp_OBJS) $(LIB_DIR)/libosal.so

---
错误3：未定义的符号

错误信息：
undefined reference to `osal_thread_create'

解决方案：
# 1. 检查符号是否存在
nm lib/libosal.so | grep osal_thread_create

# 2. 检查链接顺序（被依赖的库放后面）
LDFLAGS := -lmyapp -losal  # ✅ 正确
LDFLAGS := -losal -lmyapp  # ❌ .错误

# 3. 检查是否使用了 --no-as-needed
LDFLAGS += -Wl,--no-as-needed -losal

---
错误4：循环依赖

错误信息：
make: Circular dependency dropped: lib/liba.so -> lib/libb.so -> lib/liba.so

解决方案：
这是设计问题，需要重构：

1. 分析依赖链：
 liba → libb → liba
 
2. 打破循环：
 - 提取公共部分到 libcommon
 - 使用接口/回调
 - 合并两个库
 
3. 重构后：
 liba → libcommon
 libb → libcommon
 
---
错误5：并行编译失败

错误信息：
make: *** [lib/libhal.so] Error 1
（但串行编译成功）

解决方案：
# 检查依赖关系是否完整
$(LIB_DIR)/libhal.so: $(hal_OBJS) $(LIB_DIR)/libosal.so
#                                  ^^^^^^^^^^^^^^^^^^^^
#                                  必须显式声明依赖

# 调试方法：
make -j1  # 串行编译
make -j8  # 并行编译
# 如果串行成功，并行失败，说明依赖关系有问题

---
错误6：增量编译不工作

问题：
修改了头文件，但没有重新编译依赖的 .c 文件

解决方案：
# 1. 检查是否生成了依赖文件
ls -la .deps/

# 2. 检查编译命令是否包含 -MMD -MP
$(OBJ_DIR)/%.o: %.c
    $(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c -o $@ $<
    #                ^^^^^^^^
    #                必须包含这两个标志
    
# 3. 检查是否包含了依赖文件
-include $(wildcard $(DEP_DIR)/**/*.d)

---
E.2 调试技巧

技巧1：查看 Make 的执行过程

# 显示详细命令
make V=1

# 显示 Make 的决策过程
make -d

# 只显示要执行的命令（不实际执行）
make -n

# 显示依赖关系
make -p

技巧2：检查变量值

# 在 Makefile 中添加：
$(info osal_SRCS = $(osal_SRCS))
$(info osal_OBJS = $(osal_OBJS))

# 或使用 debug-vars 目标
.PHONY: debug-vars
debug-vars:
    @echo "osal_SRCS: $(osal_SRCS)"
    @echo "osal_OBJS: $(osal_OBJS)"

技巧3：单独编译某个文件

# 编译单个 .o 文件
make .obj/core/osal/src/posix/lib/osal_thread.o V=1

# 查看预处理结果
gcc -E core/osal/src/posix/lib/osal_thread.c -I include/osal

# 查看汇编代码
gcc -S core/osal/src/posix/lib/osal_thread.c -I include/osal

技巧4：分析依赖图

# 使用 graphviz 可视化依赖关系
make -Bnd | make2graph | dot -Tpng -o deps.png

# 或使用简单的文本输出
make debug-deps

技巧5：性能分析

# 使用 time 测量
time make -j8

# 使用 perf 分析
perf record make -j8
perf report

# 使用 strace 跟踪系统调用
strace -c make -j8

---
附录F：参考资料

F.1 官方文档

- GNU Make Manual (https://www.gnu.org/software/make/manual/)
- Linux Kernel Kbuild (https://www.kernel.org/doc/html/latest/kbuild/index.html)
- GCC Manual (https://gcc.gnu.org/onlinedocs/)

F.2 推荐阅读

- 《Recursive Make Considered Harmful》 by Peter Miller
- 经典论文，解释了递归 Make 的问题
- http://aegis.sourceforge.net/auug97.pdf
- 《Managing Projects with GNU Make》 by Robert Mecklenburg
- GNU Make 权威指南
- 包含非递归 Make 的详细讲解
- 《Advanced Linux Programming》
- 第11章：构建系统
- 包含 Make、Autotools、CMake 对比

F.3 开源项目参考

- Linux Kernel
- 混合模式（核心非递归 + 驱动递归）
- https://github.com/torvalds/linux
- Android AOSP
- 完全非递归（Soong 构建系统）
- https://source.android.com/setup/build
- Chromium
- 完全非递归（GN + Ninja）
- https://chromium.googlesource.com/chromium/src
- U-Boot
- Kbuild 风格（类似 Linux）
- https://github.com/u-boot/u-boot

F.4 工具链接

- ccache: https://ccache.dev/
- distcc: https://github.com/distcc/distcc
- Ninja: https://ninja-build.org/
- CMake: https://cmake.org/
- Meson: https://mesonbuild.com/

---
结语

这份完整的迁移方案涵盖了从规划到实施的所有细节。关键要点：

1. 渐进式迁移：分4个阶段，每个阶段都有明确的目标和验证
2. 充分测试：每个阶段都要进行功能测试、性能测试、压力测试
3. 风险管理：保留回退方案，降低迁移风险
4. 团队协作：详细文档、培训、代码审查

预期收益：
- 编译速度提升 30-60%
- 充分利用多核 CPU
- 依赖关系清晰 
- 为大规模扩展做好准备

下一步行动：
1. 保存这份文档到项目中
2. 与团队讨论迁移计划
3. 开始阶段1：基础设施搭建
4. 持续跟踪进度和问题

祝迁移顺利！🚀

✻ Worked for 1m 25s

1 tasks (0 done, 1 in progress, 0 open)
◼ 制定非递归 Make 完整迁移方案

────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯  
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ctrl+t to hide tasks


```