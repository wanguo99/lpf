# =============================================================================
# H200 AM625 产品配置构建 - Kbuild 框架
# =============================================================================

# 定义库目标
lib-y += h200_am625
so-y += h200_am625

# 源文件列表
h200_am625-objs := \
	acl/ccm_acl_config.o \
	acl/H200_100P/acl_h200_100p.o \
	acl/H200_100P/acl_h200_100p_invalidation.o \
	pcl/pcl_h200_100p_base.o \
	pcl/pcl_h200_100p_v1.o \
	pcl/pcl_h200_100p_v2.o

# 编译标志
ccflags-y := -I$(srctree)/products/ccm/h200_am625
ccflags-y += -I$(srctree)/products/ccm/h200_am625/acl
ccflags-y += -I$(srctree)/core/osal/include
ccflags-y += -I$(srctree)/core/hal/include
ccflags-y += -I$(srctree)/core/pcl/include
ccflags-y += -I$(srctree)/core/pcl/include/api
ccflags-y += -I$(srctree)/core/pdl/include
ccflags-y += -I$(srctree)/core/acl/include
ccflags-y += -I$(srctree)/include

# 链接库
LDLIBS := -L$(objtree)/lib -lacl -lpdl -lpcl -lhal -losal
