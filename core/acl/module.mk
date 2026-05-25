# =============================================================================
# ACL 模块非递归 Make 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 基础源文件
# -----------------------------------------------------------------------------
acl_SRCS := \
	core/acl/src/acl_api.c \
	core/acl/src/acl_api_v2.c \
	core/acl/src/acl_telemetry_cache.c

# 注意：acl_api_v2.c 提供 V2 版本的 API（函数名带 _V2 后缀）
# 可以与 acl_api.c 共存，用于测试或对比不同实现

# -----------------------------------------------------------------------------
# 2. 编译标志
# -----------------------------------------------------------------------------
acl_CFLAGS := \
	-Icore/acl/include \
	-Iinclude/acl \
	-Iinclude/pdl \
	-Iinclude/pcl \
	-Iinclude/pcl/api \
	-Iinclude/hal \
	-Iinclude/osal

# -----------------------------------------------------------------------------
# 3. 链接标志
# -----------------------------------------------------------------------------
acl_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-lpdl \
	-lpcl \
	-lhal \
	-losal \
	-Wl,--as-needed \
	-lpthread

# -----------------------------------------------------------------------------
# 4. 生成目标文件列表
# -----------------------------------------------------------------------------
acl_OBJS := $(call srcs_to_objs,$(acl_SRCS))

# -----------------------------------------------------------------------------
# 5. 定义目标
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_ACL_BUILD_SHARED),y)
acl_TARGET := $(STAGING_DIR)/lib/libacl.so
ALL_TARGETS += $(acl_TARGET)
endif

ifeq ($(CONFIG_ACL_BUILD_STATIC),y)
acl_STATIC_TARGET := $(STAGING_DIR)/lib/libacl.a
ALL_TARGETS += $(acl_STATIC_TARGET)
endif

# -----------------------------------------------------------------------------
# 6. 添加编译标志到全局
# -----------------------------------------------------------------------------
$(acl_OBJS): CFLAGS += $(acl_CFLAGS)

# -----------------------------------------------------------------------------
# 7. 定义构建规则
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_ACL_BUILD_SHARED),y)
$(eval $(call build_shared_lib,$(acl_TARGET),$(acl_OBJS),$(acl_LDFLAGS)))
endif

ifeq ($(CONFIG_ACL_BUILD_STATIC),y)
$(eval $(call build_static_lib,$(acl_STATIC_TARGET),$(acl_OBJS)))
endif

# -----------------------------------------------------------------------------
# 8. 依赖关系
# -----------------------------------------------------------------------------
# acl 依赖 pdl, pcl, hal, osal
$(acl_TARGET): $(STAGING_DIR)/lib/libpdl.so $(STAGING_DIR)/lib/libpcl.so $(STAGING_DIR)/lib/libhal.so $(STAGING_DIR)/lib/libosal.so
$(acl_STATIC_TARGET): $(STAGING_DIR)/lib/libpdl.so $(STAGING_DIR)/lib/libpcl.so $(STAGING_DIR)/lib/libhal.so $(STAGING_DIR)/lib/libosal.so

# -----------------------------------------------------------------------------
# 9. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(acl_OBJS) $(acl_TARGET) $(acl_STATIC_TARGET)
