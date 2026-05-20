# ============================================================================
# PCL 模块构建 - 使用通用库构建框架
# ============================================================================

# 库配置
LIB_NAME := pcl
# LIB_TYPE := static  # 取消注释可覆盖全局配置

# 源文件目录
SRC_DIR := $(srctree)/core/$(LIB_NAME)/src

# 源文件列表
SRCS := \
	pcl_api.c \
	pcl_register.c

# 导出的头文件
EXPORT_HEADERS := \
	pcl.h \
	pcl_types.h

# 头文件目录
INC_DIR := $(srctree)/core/$(LIB_NAME)/include

# 编译标志
CFLAGS_LIB := -I$(INC_DIR)
CFLAGS_LIB += -I$(INC_DIR)/api
CFLAGS_LIB += -I$(srctree)/core/hal/include
CFLAGS_LIB += -I$(srctree)/core/osal/include
CFLAGS_LIB += -I$(srctree)/include

# 链接库
LDLIBS := -lhal -losal

# 包含通用库构建框架
include $(srctree)/scripts/Makefile.lib
