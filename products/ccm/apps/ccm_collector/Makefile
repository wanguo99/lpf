# ============================================================================
# CCM Collector Application - Kbuild 框架
# ============================================================================

# 包含 Kconfig 配置
-include $(objtree)/include/config/auto.conf

# 应用名称
APP_NAME := ccm_collector

# 定义可执行文件目标
obj-e := ccm_collector

# 子目录
subdir-y += src/

# 私有编译标志（每个应用独立定义）
CFLAGS_PRIV := -I$(srctree)/products/ccm/apps/ccm_collector/include
CFLAGS_PRIV += -I$(srctree)/core/osal/include
CFLAGS_PRIV += -I$(srctree)/core/hal/include
CFLAGS_PRIV += -I$(srctree)/core/pcl/include
CFLAGS_PRIV += -I$(srctree)/core/pcl/include/api
CFLAGS_PRIV += -I$(srctree)/core/pdl/include
CFLAGS_PRIV += -I$(srctree)/core/acl/include
CFLAGS_PRIV += -I$(srctree)/products/ccm/libs/libccm/include
CFLAGS_PRIV += -I$(srctree)/include

# 私有链接标志和库（每个应用独立定义）
# 链接方式：静态链接
LDFLAGS_PRIV := -L$(objtree)/lib
LDFLAGS_PRIV += -static

LDLIBS_PRIV := -lh200_am625 -lccm
LDLIBS_PRIV += -lacl -lpdl -lpcl -lhal -losal
LDLIBS_PRIV += -lpthread -lrt

# 包含 Kbuild 构建框架
include $(scriptdir)/build/Makefile.build

