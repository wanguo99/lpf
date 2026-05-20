# ============================================================================
# CCM Health Application - Kbuild 框架
# ============================================================================

# 定义可执行文件目标
obj-e := ccm_health

# 子目录
subdir-y += src/

# 编译标志
CFLAGS_APP := -I$(srctree)/products/ccm/apps/ccm_health/include
CFLAGS_APP += -I$(srctree)/core/osal/include
CFLAGS_APP += -I$(srctree)/core/hal/include
CFLAGS_APP += -I$(srctree)/core/pcl/include
CFLAGS_APP += -I$(srctree)/core/pcl/include/api
CFLAGS_APP += -I$(srctree)/core/pdl/include
CFLAGS_APP += -I$(srctree)/core/acl/include
CFLAGS_APP += -I$(srctree)/products/ccm/libs/libccm/include
CFLAGS_APP += -I$(srctree)/include

# 链接库
LDLIBS := -L$(objtree)/lib
LDLIBS += -lh200_am625 -lccm
LDLIBS += -lacl -lpdl -lpcl -lhal -losal
LDLIBS += -lpthread -lrt

# 包含 Kbuild 构建框架
include $(scriptdir)/build/Makefile.build
