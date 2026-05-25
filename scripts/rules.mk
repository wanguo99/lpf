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
-include $(shell find $(BUILD_DIR) -name '*.d' 2>/dev/null)

# -----------------------------------------------------------------------------
# 规则 2：.o → .so（动态库）
# -----------------------------------------------------------------------------
# 使用模式：$(call build_shared_lib,target,objs,ldflags)
define build_shared_lib
$(1): $(2)
	@echo "  LD      $$@"
	@mkdir -p $$(dir $$@)
	@$$(CC) -shared -o $$@ $$^ $(3)
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
