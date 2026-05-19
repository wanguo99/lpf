# ============================================================================
# PCL 模块构建
# ============================================================================

# 必须在包含 Makefile.lib 之前定义
GENERATED_DIR ?= $(objtree)/include/generated

include $(srctree)/scripts/Makefile.lib

# 模块名称
MODULE := pcl
LIB_NAME := libpcl.so

# 输出目录
BUILD_DIR ?= $(objtree)/core/$(MODULE)
LIB_DIR := $(objtree)/lib

# 源文件目录
SRC_DIR := $(srctree)/core/$(MODULE)/src
INC_DIR := $(srctree)/core/$(MODULE)/include

# 源文件列表
SRCS := \
	pcl_api.c \
	pcl_register.c

# 目标文件
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# 编译标志
CFLAGS_MODULE := -I$(INC_DIR)
CFLAGS_MODULE += -I$(INC_DIR)/api
CFLAGS_MODULE += -I$(srctree)/core/osal/include
CFLAGS_MODULE += -I$(srctree)/core/hal/include

# 链接库
LDLIBS := -L$(LIB_DIR) -losal

# 默认目标
.PHONY: all clean

all: $(LIB_DIR)/$(LIB_NAME)
	@echo "PCL module built successfully"

# 编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(call cmd,cc_o_c)

# 链接规则
$(LIB_DIR)/$(LIB_NAME): $(OBJS) | $(LIB_DIR)
	$(call cmd,ld_so)

# 创建输出目录
$(LIB_DIR):
	@mkdir -p $@

# 清理
# 包含依赖文件（自动跟踪头文件变化）
-include $(OBJS:.o=.d)

clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(LIB_DIR)/$(LIB_NAME)
