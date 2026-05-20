# ============================================================================
# 通用应用程序构建框架 (Makefile.app)
# ============================================================================
# 用于构建可执行文件
#
# 使用方法：
# 1. 在模块 Makefile 中定义以下变量：
#    - APP_NAME: 应用程序名称（如 sample_app）
#    - SRCS: 源文件列表（相对于 SRC_DIR）
#    - CFLAGS_APP: 应用特定的编译标志
#    - LDFLAGS: 链接标志
#    - LDLIBS: 链接库
#
# 2. 包含此文件：include $(srctree)/scripts/Makefile.app
#
# 示例：
#   APP_NAME := sample_app
#   SRCS := main.c app_logic.c
#   LDLIBS := -L$(objtree)/lib -losal -lhal
#   include $(srctree)/scripts/Makefile.app
# ============================================================================

ifndef APP_NAME
$(error APP_NAME is not defined. Please define it before including Makefile.app)
endif

# 包含通用规则库
include $(srctree)/scripts/Makefile.rules

# ============================================================================
# 默认值和路径设置
# ============================================================================

# 源文件目录（默认为当前应用的 src/）
SRC_DIR ?= $(srctree)/products/$(APP_NAME)/src

# 头文件目录（默认为当前应用的 include/）
INC_DIR ?= $(srctree)/products/$(APP_NAME)/include

# 构建输出目录
BUILD_DIR ?= $(objtree)/products/$(APP_NAME)

# 可执行文件输出目录
BIN_DIR := $(objtree)/bin

# 目标可执行文件
TARGET := $(BIN_DIR)/$(APP_NAME)

# ============================================================================
# 源文件和目标文件处理
# ============================================================================

# 支持条件编译：SRCS-$(CONFIG_XXX) += file.c
SRCS_ALL := $(SRCS)
SRCS_ALL += $(foreach cfg,$(filter CONFIG_%,$(.VARIABLES)),$(SRCS-$(cfg)))

# 将源文件转换为目标文件路径
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS_ALL))

# ============================================================================
# 编译标志
# ============================================================================

# 添加应用头文件路径
CFLAGS_APP += -I$(INC_DIR)

# 添加顶层 include 路径（用于引用库头文件）
CFLAGS_APP += -I$(srctree)/include

# 链接标志（默认添加 rpath）
LDFLAGS += -Wl,-rpath,$(objtree)/lib

# ============================================================================
# 构建目标
# ============================================================================

.PHONY: all clean install

all: $(TARGET)

# 链接可执行文件
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(call cmd,ld_exe)

# 编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(call cmd,cc_o_c_app)

# 创建输出目录
$(BIN_DIR):
	@mkdir -p $@

# ============================================================================
# 安装
# ============================================================================

install:
	@echo "  INSTALL $(APP_NAME)"
	@mkdir -p $(INSTALL_BIN_PATH)
	@cp -f $(TARGET) $(INSTALL_BIN_PATH)/

# ============================================================================
# 清理
# ============================================================================

clean:
	@echo "  CLEAN   $(APP_NAME)"
	@rm -rf $(BUILD_DIR)
	@rm -f $(TARGET)

# ============================================================================
# 依赖跟踪
# ============================================================================

# 包含自动生成的依赖文件（.d 文件）
-include $(OBJS:.o=.d)
