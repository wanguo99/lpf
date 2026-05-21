# ============================================================================
# PCL 模块构建 - Kbuild 风格
# ============================================================================

# 包含 Kconfig 配置
-include $(objtree)/include/config/auto.conf

# 库名称
LIB_NAME := pcl

# 根据 Kconfig 配置决定构建目标
ifeq ($(CONFIG_PCL_BUILD_STATIC),y)
  obj-s := libpcl.a
endif

ifeq ($(CONFIG_PCL_BUILD_SHARED),y)
  obj-d := libpcl.so
endif

# 对象文件列表（Linux 内核风格）
libpcl.a-objs := \
	src/pcl_api.o \
	src/pcl_register.o

# 动态库使用相同的对象文件
libpcl.so-objs := $(libpcl.a-objs)

# 私有编译标志（每个模块独立定义）
CFLAGS_PRIV := -I$(srctree)/core/pcl/include
CFLAGS_PRIV += -I$(srctree)/core/pcl/include/api
CFLAGS_PRIV += -I$(srctree)/core/hal/include
CFLAGS_PRIV += -I$(srctree)/core/osal/include
CFLAGS_PRIV += -I$(srctree)/include

# 私有链接标志和库（每个模块独立定义）
LDFLAGS_PRIV :=
LDLIBS_PRIV := -L$(objtree)/lib -lhal -losal

# 包含统一构建框架
include $(scriptdir)/build/Makefile.build
