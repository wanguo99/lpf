# =============================================================================
# PCL 模块构建配置（非递归 Make）
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 包含源文件配置
# -----------------------------------------------------------------------------
pcl_SRCS :=
include core/pcl/src/module.mk

# -----------------------------------------------------------------------------
# 2. 编译标志
# -----------------------------------------------------------------------------
pcl_CFLAGS := \
	-Iinclude/pcl \
	-Iinclude/pcl/api \
	-Iinclude/hal \
	-Iinclude/osal

# -----------------------------------------------------------------------------
# 3. 链接标志
# -----------------------------------------------------------------------------
pcl_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,-soname,libpcl.so.1 \
	-Wl,--no-as-needed \
	-lhal \
	-losal \
	-Wl,--as-needed

# -----------------------------------------------------------------------------
# 以下为标准构建流程
# -----------------------------------------------------------------------------

pcl_OBJS := $(call srcs_to_objs,$(pcl_SRCS))

ifeq ($(CONFIG_PCL),y)
  ifeq ($(CONFIG_PCL_BUILD_SHARED),y)
    pcl_SO_TARGET := $(STAGING_DIR)/lib/libpcl.so
    ALL_TARGETS += $(pcl_SO_TARGET)
  endif

  ifeq ($(CONFIG_PCL_BUILD_STATIC),y)
    pcl_A_TARGET := $(STAGING_DIR)/lib/libpcl.a
    ALL_TARGETS += $(pcl_A_TARGET)
  endif
endif

$(pcl_OBJS): CFLAGS += $(pcl_CFLAGS)

ifeq ($(CONFIG_PCL),y)

ifeq ($(CONFIG_PCL_BUILD_SHARED),y)
$(pcl_SO_TARGET): $(pcl_OBJS)
$(pcl_SO_TARGET): $(STAGING_DIR)/lib/libhal.so $(STAGING_DIR)/lib/libosal.so

$(pcl_SO_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -shared -o $@ $(pcl_OBJS) $(pcl_LDFLAGS)
	@if [ -n "$(pcl_LDFLAGS)" ] && echo "$(pcl_LDFLAGS)" | grep -q "soname,"; then \
		soname=$$(echo "$(pcl_LDFLAGS)" | sed -n 's/.*-soname,\([^ ]*\).*/\1/p'); \
		if [ -n "$$soname" ] && [ "$$soname" != "$$(basename $@)" ]; then \
			ln -sf $$(basename $@) $$(dirname $@)/$$soname; \
		fi; \
	fi
endif

ifeq ($(CONFIG_PCL_BUILD_STATIC),y)
$(pcl_A_TARGET): $(pcl_OBJS)

$(pcl_A_TARGET):
	@echo "  AR      $@"
	@mkdir -p $(dir $@)
	@rm -f $@
	@ar rcs $@ $(pcl_OBJS)
endif

endif

CLEAN_TARGETS += $(pcl_OBJS) $(pcl_SO_TARGET) $(pcl_A_TARGET)
