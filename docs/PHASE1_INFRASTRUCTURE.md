# 阶段 1：基础设施搭建

> **预计时间**: 3天  
> **目标**: 搭建非递归 Make 框架，验证 Kconfig 兼容性

---

## 📋 任务清单

- [ ] 1.1 创建 `scripts/rules.mk`（通用编译规则）
- [ ] 1.2 创建 `scripts/functions.mk`（辅助函数）
- [ ] 1.3 创建 `scripts/auto-module.sh`（自动生成工具）
- [ ] 1.4 备份当前 Makefile
- [ ] 1.5 创建新的顶层 Makefile（框架）
- [ ] 1.6 验证 Kconfig 集成
- [ ] 1.7 测试基本功能

---

## 1.1 创建 scripts/rules.mk

**作用**：定义通用的编译规则（.c → .o, .o → .so, .o → bin）

**文件内容**：

```makefile
# =============================================================================
# 通用编译规则
# =============================================================================

# 构建目录
BUILD_DIR := build
STAGING_DIR := .staging

# 编译器标志（从 Kconfig 读取）
CFLAGS := -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs \
          -fno-strict-aliasing -fno-common \
          -Werror-implicit-function-declaration \
          -Wno-format-security -std=gnu89 -O2 \
          -fomit-frame-pointer -fPIC

# 包含 Kconfig 生成的配置
CFLAGS += -I$(STAGING_DIR)/include
CFLAGS += -include include/generated/autoconf.h
CFLAGS += -D__EMS__

# 链接器标志
LDFLAGS := -shared -L$(STAGING_DIR)/lib

# -----------------------------------------------------------------------------
# 规则 1：.c → .o（带自动依赖生成）
# -----------------------------------------------------------------------------
$(BUILD_DIR)/%.o: %.c
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# 包含自动生成的依赖文件
-include $(BUILD_DIR)/**/*.d

# -----------------------------------------------------------------------------
# 规则 2：.o → .so（动态库）
# -----------------------------------------------------------------------------
# 使用模式：$(call build_shared_lib,target,objs,ldflags)
define build_shared_lib
$(1): $(2)
	@echo "  LD      $$@"
	@mkdir -p $$(dir $$@)
	@$$(CC) -shared -o $$@ $$^ $(3)
	@echo "  INSTALL $$@"
	@mkdir -p $(STAGING_DIR)/lib
	@cp $$@ $(STAGING_DIR)/lib/
endef

# -----------------------------------------------------------------------------
# 规则 3：.o → .a（静态库）
# -----------------------------------------------------------------------------
define build_static_lib
$(1): $(2)
	@echo "  AR      $$@"
	@mkdir -p $$(dir $$@)
	@rm -f $$@
	@ar rcs $$@ $$^
	@echo "  INSTALL $$@"
	@mkdir -p $(STAGING_DIR)/lib
	@cp $$@ $(STAGING_DIR)/lib/
endef

# -----------------------------------------------------------------------------
# 规则 4：.o → bin（可执行文件）
# -----------------------------------------------------------------------------
define build_executable
$(1): $(2)
	@echo "  LD      $$@"
	@mkdir -p $$(dir $$@)
	@$$(CC) -o $$@ $$^ $(3)
	@echo "  INSTALL $$@"
	@mkdir -p $(STAGING_DIR)/bin
	@cp $$@ $(STAGING_DIR)/bin/
endef

# -----------------------------------------------------------------------------
# 清理规则
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@echo "  CLEAN   $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)
	@echo "  CLEAN   $(STAGING_DIR)"
	@rm -rf $(STAGING_DIR)
	@echo "  CLEAN   include/generated"
	@rm -rf include/generated

# -----------------------------------------------------------------------------
# 帮助信息
# -----------------------------------------------------------------------------
.PHONY: help
help:
	@echo "EMS 非递归 Make 构建系统"
	@echo ""
	@echo "常用目标："
	@echo "  make              - 编译所有模块"
	@echo "  make clean        - 清理构建产物"
	@echo "  make menuconfig   - 配置系统"
	@echo "  make help         - 显示此帮助"
	@echo ""
	@echo "并行编译："
	@echo "  make -j24         - 使用 24 个并行任务"
	@echo "  make -j\$$(nproc)   - 使用所有 CPU 核心"
```

---

## 1.2 创建 scripts/functions.mk

**作用**：提供辅助函数（路径转换、依赖生成等）

**文件内容**：

```makefile
# =============================================================================
# 辅助函数
# =============================================================================

# 函数：将源文件路径转换为目标文件路径
# 用法：$(call src_to_obj,src/file.c)
# 结果：build/src/file.o
src_to_obj = $(patsubst %.c,$(BUILD_DIR)/%.o,$(1))

# 函数：将源文件列表转换为目标文件列表
# 用法：$(call srcs_to_objs,$(SRCS))
srcs_to_objs = $(foreach src,$(1),$(call src_to_obj,$(src)))

# 函数：提取模块名
# 用法：$(call get_module_name,core/osal)
# 结果：osal
get_module_name = $(notdir $(1))

# 函数：生成库文件名
# 用法：$(call get_lib_name,osal)
# 结果：libosal.so
get_lib_name = lib$(1).so

# 函数：生成静态库文件名
# 用法：$(call get_static_lib_name,osal)
# 结果：libosal.a
get_static_lib_name = lib$(1).a

# 函数：检查配置选项
# 用法：$(call check_config,CONFIG_OSAL)
# 结果：如果 CONFIG_OSAL=y，返回 y，否则返回空
check_config = $(filter y,$($(1)))
```

---

## 1.3 创建 scripts/auto-module.sh

**作用**：自动从现有 Makefile 生成 module.mk

**文件内容**：

```bash
#!/bin/bash
# =============================================================================
# 自动生成 module.mk
# =============================================================================

set -e

if [ $# -ne 1 ]; then
    echo "用法: $0 <模块目录>"
    echo "示例: $0 core/osal"
    exit 1
fi

MODULE_DIR="$1"
MODULE_NAME=$(basename "$MODULE_DIR")
MODULE_MK="${MODULE_DIR}/module.mk"

if [ ! -d "$MODULE_DIR" ]; then
    echo "错误：目录不存在: $MODULE_DIR"
    exit 1
fi

if [ ! -f "${MODULE_DIR}/Makefile" ]; then
    echo "错误：Makefile 不存在: ${MODULE_DIR}/Makefile"
    exit 1
fi

echo "正在生成 ${MODULE_MK}..."

# 提取源文件列表
SRCS=$(grep -E "^obj-y.*\.o" "${MODULE_DIR}/Makefile" | \
       sed 's/obj-y += //' | \
       sed 's/\.o/.c/' | \
       tr '\n' ' ')

# 提取编译标志
CFLAGS=$(grep -E "^ccflags-y" "${MODULE_DIR}/Makefile" | \
         sed 's/ccflags-y += //' | \
         tr '\n' ' ')

# 提取链接标志
LDFLAGS=$(grep -E "^ldflags-y" "${MODULE_DIR}/Makefile" | \
          sed 's/ldflags-y += //' | \
          tr '\n' ' ')

# 生成 module.mk
cat > "$MODULE_MK" << EOF
# =============================================================================
# ${MODULE_NAME} 模块构建配置
# =============================================================================
# 自动生成于: $(date)
# 生成工具: scripts/auto-module.sh

# 模块名称
${MODULE_NAME}_MODULE := ${MODULE_DIR}

# 源文件列表
${MODULE_NAME}_SRCS := \\
${SRCS}

# 目标文件列表
${MODULE_NAME}_OBJS := \$(call srcs_to_objs,\$(${MODULE_NAME}_SRCS))

# 编译标志
${MODULE_NAME}_CFLAGS := ${CFLAGS}

# 链接标志
${MODULE_NAME}_LDFLAGS := ${LDFLAGS}

# 目标库
${MODULE_NAME}_TARGET := \$(STAGING_DIR)/lib/lib${MODULE_NAME}.so
${MODULE_NAME}_STATIC := \$(STAGING_DIR)/lib/lib${MODULE_NAME}.a

# 添加到全局目标列表
ALL_TARGETS += \$(${MODULE_NAME}_TARGET)

# 定义构建规则
\$(eval \$(call build_shared_lib,\$(${MODULE_NAME}_TARGET),\$(${MODULE_NAME}_OBJS),\$(${MODULE_NAME}_LDFLAGS)))
\$(eval \$(call build_static_lib,\$(${MODULE_NAME}_STATIC),\$(${MODULE_NAME}_OBJS)))

# 编译标志（针对此模块的 .o 文件）
\$(${MODULE_NAME}_OBJS): CFLAGS += \$(${MODULE_NAME}_CFLAGS)
EOF

echo "✓ 生成完成: ${MODULE_MK}"
echo ""
echo "请检查并手动调整："
echo "  1. 源文件列表是否完整"
echo "  2. 编译标志是否正确"
echo "  3. 依赖关系是否正确"
```

---

## 1.4 备份当前 Makefile

```bash
cp Makefile Makefile.recursive
git add Makefile.recursive
git commit -m "备份：保存递归 Make 的 Makefile"
```

---

## 1.5 创建新的顶层 Makefile

**文件内容**：

```makefile
# =============================================================================
# EMS 非递归 Make 构建系统
# =============================================================================
# 版本: 2.0
# 创建日期: 2026-05-25

# 默认目标
.DEFAULT_GOAL := all

# 包含 Kconfig 生成的配置
-include .config

# 包含辅助函数和规则
include scripts/functions.mk
include scripts/rules.mk

# 全局目标列表
ALL_TARGETS :=

# =============================================================================
# Core 模块（根据 Kconfig 配置包含）
# =============================================================================

# 暂时为空，阶段 2 会添加
# ifeq ($(CONFIG_OSAL),y)
#     include core/osal/module.mk
# endif

# =============================================================================
# Products 模块
# =============================================================================

# 暂时为空，阶段 3 会添加

# =============================================================================
# 主目标
# =============================================================================

.PHONY: all
all: $(ALL_TARGETS)
	@echo "  BUILD   EMS $(VERSION)"

# =============================================================================
# Kconfig 目标（保持和递归 Make 相同）
# =============================================================================

%config: scripts/kconfig/conf
	@$< --$@ Kconfig

scripts/kconfig/conf:
	@$(MAKE) -C scripts/kconfig conf

# =============================================================================
# 版本信息
# =============================================================================

VERSION := 1.0.0
export VERSION
```

---

## 1.6 验证 Kconfig 集成

```bash
# 测试 menuconfig
make menuconfig

# 测试 defconfig
make ccm_h200_am625_debug_defconfig

# 检查 .config 生成
cat .config | grep CONFIG_OSAL
# 应该输出：CONFIG_OSAL=y
```

---

## 1.7 测试基本功能

```bash
# 测试 help
make help

# 测试 clean
make clean

# 测试 all（当前应该什么都不做）
make all
# 应该输出：BUILD   EMS 1.0.0
```

---

## ✅ 验证清单

完成以下验证后，才能进入阶段 2：

- [ ] `scripts/rules.mk` 创建成功
- [ ] `scripts/functions.mk` 创建成功
- [ ] `scripts/auto-module.sh` 创建成功且可执行
- [ ] `Makefile.recursive` 备份成功
- [ ] 新的 `Makefile` 创建成功
- [ ] `make menuconfig` 正常工作
- [ ] `make ccm_h200_am625_debug_defconfig` 正常工作
- [ ] `.config` 文件正常生成
- [ ] `make help` 显示帮助信息
- [ ] `make clean` 正常工作
- [ ] `make all` 正常执行（虽然什么都不编译）

---

## 🚀 准备好了吗？

验证通过后，执行：

```bash
git add scripts/rules.mk scripts/functions.mk scripts/auto-module.sh Makefile
git commit -m "阶段1：搭建非递归 Make 基础设施"
```

然后进入 **阶段 2：Core 层迁移**！
