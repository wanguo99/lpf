# =============================================================================
# libh200_am625 模块非递归 Make 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 基础源文件
# -----------------------------------------------------------------------------
libh200_am625_SRCS := \
	products/ccm/h200_am625/acl/ccm_acl_config.c \
	products/ccm/h200_am625/acl/H200_100P/acl_h200_100p.c \
	products/ccm/h200_am625/acl/H200_100P/acl_h200_100p_invalidation.c \
	products/ccm/h200_am625/pcl/pcl_h200_100p_base.c \
	products/ccm/h200_am625/pcl/pcl_h200_100p_v1.c \
	products/ccm/h200_am625/pcl/pcl_h200_100p_v2.c

# -----------------------------------------------------------------------------
# 2. 编译标志
# -----------------------------------------------------------------------------
libh200_am625_CFLAGS := \
	-Iproducts/ccm/h200_am625/include \
	-Iproducts/ccm/h200_am625/acl \
	-Iproducts/ccm/h200_am625/acl/H200_100P \
	-Iinclude/acl \
	-Iinclude/pdl \
	-Iinclude/pcl \
	-Iinclude/pcl/api \
	-Iinclude/hal \
	-Iinclude/osal

# -----------------------------------------------------------------------------
# 3. 链接标志
# -----------------------------------------------------------------------------
libh200_am625_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed

# 根据启用的核心模块链接对应的库
ifeq ($(CONFIG_ACL),y)
libh200_am625_LDFLAGS += -lacl
endif

ifeq ($(CONFIG_PDL),y)
libh200_am625_LDFLAGS += -lpdl
endif

ifeq ($(CONFIG_PCL),y)
libh200_am625_LDFLAGS += -lpcl
endif

ifeq ($(CONFIG_HAL),y)
libh200_am625_LDFLAGS += -lhal
endif

ifeq ($(CONFIG_OSAL),y)
libh200_am625_LDFLAGS += -losal
endif

libh200_am625_LDFLAGS += -Wl,--as-needed -lpthread

# -----------------------------------------------------------------------------
# 4. 生成目标文件列表
# -----------------------------------------------------------------------------
libh200_am625_OBJS := $(call srcs_to_objs,$(libh200_am625_SRCS))

# -----------------------------------------------------------------------------
# 5. 定义目标
# -----------------------------------------------------------------------------
libh200_am625_TARGET := $(STAGING_DIR)/lib/libh200_am625.so
ALL_TARGETS += $(libh200_am625_TARGET)

# -----------------------------------------------------------------------------
# 6. 添加编译标志到全局
# -----------------------------------------------------------------------------
$(libh200_am625_OBJS): CFLAGS += $(libh200_am625_CFLAGS)

# -----------------------------------------------------------------------------
# 7. 定义构建规则
# -----------------------------------------------------------------------------
$(eval $(call build_shared_lib,$(libh200_am625_TARGET),$(libh200_am625_OBJS),$(libh200_am625_LDFLAGS)))

# -----------------------------------------------------------------------------
# 8. 依赖关系
# -----------------------------------------------------------------------------
# libh200_am625 依赖所有启用的 Core 库
ifeq ($(CONFIG_ACL),y)
$(libh200_am625_TARGET): $(STAGING_DIR)/lib/libacl.so
endif

ifeq ($(CONFIG_PDL),y)
$(libh200_am625_TARGET): $(STAGING_DIR)/lib/libpdl.so
endif

ifeq ($(CONFIG_PCL),y)
$(libh200_am625_TARGET): $(STAGING_DIR)/lib/libpcl.so
endif

ifeq ($(CONFIG_HAL),y)
$(libh200_am625_TARGET): $(STAGING_DIR)/lib/libhal.so
endif

ifeq ($(CONFIG_OSAL),y)
$(libh200_am625_TARGET): $(STAGING_DIR)/lib/libosal.so
endif

# -----------------------------------------------------------------------------
# 9. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(libh200_am625_OBJS) $(libh200_am625_TARGET)
