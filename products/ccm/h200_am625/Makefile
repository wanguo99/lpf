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
# 引用统一的 include 目录
ccflags-y += -I$(srctree)/include/acl
ccflags-y += -I$(srctree)/include/pdl
ccflags-y += -I$(srctree)/include/pcl
ccflags-y += -I$(srctree)/include/pcl/api
ccflags-y += -I$(srctree)/include/hal
ccflags-y += -I$(srctree)/include/hal/config
ccflags-y += -I$(srctree)/include/osal

# 链接库
LDLIBS := -L$(LIB_DIR) -lacl -lpdl -lpcl -lhal -losal
