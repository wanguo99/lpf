# ============================================================================
# PDL 模块构建 - Kbuild 风格
# ============================================================================

# 包含 Kconfig 配置
-include $(objtree)/include/config/auto.conf

# 库名称
LIB_NAME := pdl

# 根据 Kconfig 配置决定构建目标
ifeq ($(CONFIG_PDL_BUILD_STATIC),y)
  obj-s := libpdl.a
endif

ifeq ($(CONFIG_PDL_BUILD_SHARED),y)
  obj-d := libpdl.so
endif

# 子目录递归
subdir-y += src/

# 从子目录收集对象文件
libpdl.a-objs := $(obj-y)
libpdl.so-objs := $(obj-y)

# 私有编译标志（每个模块独立定义）
CFLAGS_PRIV := -I$(srctree)/core/pdl/include
CFLAGS_PRIV += -I$(srctree)/core/pcl/include
CFLAGS_PRIV += -I$(srctree)/core/hal/include
CFLAGS_PRIV += -I$(srctree)/core/osal/include
CFLAGS_PRIV += -I$(srctree)/include

# 私有链接标志和库（每个模块独立定义）
LDFLAGS_PRIV :=
LDLIBS_PRIV := -L$(objtree)/lib -lpcl -lhal -losal

# 包含统一构建框架
include $(scriptdir)/build/Makefile.build
