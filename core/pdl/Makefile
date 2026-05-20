# ============================================================================
# PDL 模块构建 - 使用通用库构建框架
# ============================================================================

# 库配置
LIB_NAME := pdl
# LIB_TYPE := static  # 取消注释可覆盖全局配置

# 源文件目录
SRC_DIR := $(srctree)/core/$(LIB_NAME)/src

# 条件编译：根据 Kconfig 配置选择源文件
SRCS-$(CONFIG_PDL_MCU_SUPPORT) += pdl_mcu/pdl_mcu.c
SRCS-$(CONFIG_PDL_SATELLITE) += pdl_satellite/pdl_satellite.c
SRCS-$(CONFIG_PDL_RECTIFIER) += pdl_rectifier/pdl_rectifier.c
SRCS-$(CONFIG_PDL_DCDC) += pdl_dcdc/pdl_dcdc.c

# 导出的头文件
EXPORT_HEADERS := \
	pdl.h \
	pdl_types.h \
	pdl_mcu/pdl_mcu.h \
	pdl_satellite/pdl_satellite.h \
	pdl_rectifier/pdl_rectifier.h \
	pdl_dcdc/pdl_dcdc.h

# 头文件目录
INC_DIR := $(srctree)/core/$(LIB_NAME)/include

# 编译标志
CFLAGS_LIB := -I$(INC_DIR)
CFLAGS_LIB += -I$(srctree)/core/pcl/include
CFLAGS_LIB += -I$(srctree)/core/hal/include
CFLAGS_LIB += -I$(srctree)/include

# 链接库
LDLIBS := -lpcl -lhal -losal

# 包含通用库构建框架
include $(scriptdir)/build/Makefile.lib
