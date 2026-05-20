# ============================================================================
# H200 AM625 产品配置构建 - 使用通用库构建框架
# ============================================================================

# 库配置
LIB_NAME := h200_am625

# 库类型：静态库
LIB_TYPE := static

# 源文件目录
SRC_DIR := $(srctree)/products/ccm/h200_am625

# PCL 源文件
SRCS := \
	pcl/pcl_h200_100p_base.c \
	pcl/pcl_h200_100p_v1.c \
	pcl/pcl_h200_100p_v2.c \
	acl/ccm_acl_config.c

# 导出的头文件
EXPORT_HEADERS := \
	h200_am625_config.h

# 头文件目录
INC_DIR := $(srctree)/products/ccm/h200_am625

# 编译标志
CFLAGS_LIB := -I$(INC_DIR)
CFLAGS_LIB += -I$(INC_DIR)/acl
CFLAGS_LIB += -I$(srctree)/core/osal/include
CFLAGS_LIB += -I$(srctree)/core/hal/include
CFLAGS_LIB += -I$(srctree)/core/pcl/include
CFLAGS_LIB += -I$(srctree)/core/pcl/include/api
CFLAGS_LIB += -I$(srctree)/core/pdl/include
CFLAGS_LIB += -I$(srctree)/core/acl/include
CFLAGS_LIB += -I$(srctree)/include

# 链接库
LDLIBS := -lacl -lpdl -lpcl -lhal -losal

# 包含通用库构建框架
include $(scriptdir)/build/Makefile.lib
