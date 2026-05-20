# ============================================================================
# ACL 模块构建 - 使用通用库构建框架
# ============================================================================

# 库配置
LIB_NAME := acl

# 库类型从 Kconfig 读取
ifdef CONFIG_ACL_LIB_TYPE_SHARED
  LIB_TYPE := shared
else ifdef CONFIG_ACL_LIB_TYPE_STATIC
  LIB_TYPE := static
else ifdef CONFIG_ACL_LIB_TYPE_BOTH
  LIB_TYPE := both
endif
# 如果未设置，使用全局默认值（在 Makefile.lib 中处理）

# 源文件目录
SRC_DIR := $(srctree)/core/$(LIB_NAME)/src

# 源文件列表
SRCS := \
	acl_api.c \
	acl_telemetry_cache.c

# 导出的头文件
EXPORT_HEADERS := \
	acl.h \
	acl_types.h

# 头文件目录
INC_DIR := $(srctree)/core/$(LIB_NAME)/include

# 编译标志
CFLAGS_LIB := -I$(INC_DIR)
CFLAGS_LIB += -I$(srctree)/core/pdl/include
CFLAGS_LIB += -I$(srctree)/core/pcl/include
CFLAGS_LIB += -I$(srctree)/core/hal/include
CFLAGS_LIB += -I$(srctree)/core/osal/include
CFLAGS_LIB += -I$(srctree)/include

# 链接库
LDLIBS := -lpdl -lpcl -lhal -losal

# 包含通用库构建框架
include $(srctree)/scripts/Makefile.lib
