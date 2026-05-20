# ============================================================================
# H200 AM625 产品配置构建 - Kbuild 框架
# ============================================================================

# 定义库目标
obj-s := libh200_am625.a
obj-d := libh200_am625.so

# 子目录
subdir-y += pcl/
subdir-y += acl/

# 编译标志
CFLAGS_LIB := -I$(srctree)/products/ccm/h200_am625
CFLAGS_LIB += -I$(srctree)/products/ccm/h200_am625/acl
CFLAGS_LIB += -I$(srctree)/core/osal/include
CFLAGS_LIB += -I$(srctree)/core/hal/include
CFLAGS_LIB += -I$(srctree)/core/pcl/include
CFLAGS_LIB += -I$(srctree)/core/pcl/include/api
CFLAGS_LIB += -I$(srctree)/core/pdl/include
CFLAGS_LIB += -I$(srctree)/core/acl/include
CFLAGS_LIB += -I$(srctree)/include

# 链接库
LDLIBS := -L$(objtree)/lib -lacl -lpdl -lpcl -lhal -losal

# 包含 Kbuild 构建框架
include $(scriptdir)/build/Makefile.build

