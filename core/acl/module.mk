# =============================================================================
# ACL 模块构建配置（非递归 Make）
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 包含源文件配置
# -----------------------------------------------------------------------------
acl_SRCS :=
include core/acl/src/module.mk

# -----------------------------------------------------------------------------
# 2. 编译标志
# -----------------------------------------------------------------------------
acl_CFLAGS := \
	-Iinclude

# -----------------------------------------------------------------------------
# 3. 链接标志
# -----------------------------------------------------------------------------
acl_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,-soname,libacl.so.1 \
	-Wl,--no-as-needed \
	-lpdl \
	-lpcl \
	-lhal \
	-losal \
	-Wl,--as-needed

# -----------------------------------------------------------------------------
# 4. 导出头文件
# -----------------------------------------------------------------------------
acl_HEADERS := \
	acl_types.h \
	acl_api.h \
	acl_api_v2.h \
	acl_config.h \
	acl_tc.h \
	acl_tm.h \
	acl_telemetry_cache.h

# -----------------------------------------------------------------------------
# 以下为标准构建流程
# -----------------------------------------------------------------------------

acl_OBJS := $(call srcs_to_objs,$(acl_SRCS))

ifeq ($(CONFIG_ACL),y)
  ifeq ($(CONFIG_ACL_BUILD_SHARED),y)
    acl_SO_TARGET := $(STAGING_DIR)/lib/libacl.so
    ALL_TARGETS += $(acl_SO_TARGET)
  endif

  ifeq ($(CONFIG_ACL_BUILD_STATIC),y)
    acl_A_TARGET := $(STAGING_DIR)/lib/libacl.a
    ALL_TARGETS += $(acl_A_TARGET)
  endif
endif

$(acl_OBJS): CFLAGS += $(acl_CFLAGS)



ifeq ($(CONFIG_ACL),y)

ifeq ($(CONFIG_ACL_BUILD_SHARED),y)
$(acl_SO_TARGET): $(acl_OBJS)
$(acl_SO_TARGET): $(STAGING_DIR)/lib/libpdl.so $(STAGING_DIR)/lib/libpcl.so $(STAGING_DIR)/lib/libhal.so $(STAGING_DIR)/lib/libosal.so

$(acl_SO_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -shared -o $@ $(acl_OBJS) $(acl_LDFLAGS)
	@if [ -n "$(acl_LDFLAGS)" ] && echo "$(acl_LDFLAGS)" | grep -q "soname,"; then \
		soname=$$(echo "$(acl_LDFLAGS)" | sed -n 's/.*-soname,\([^ ]*\).*/\1/p'); \
		if [ -n "$$soname" ] && [ "$$soname" != "$$(basename $@)" ]; then \
			ln -sf $$(basename $@) $$(dirname $@)/$$soname; \
		fi; \
	fi
endif

ifeq ($(CONFIG_ACL_BUILD_STATIC),y)
$(acl_A_TARGET): $(acl_OBJS)

$(acl_A_TARGET):
	@echo "  AR      $@"
	@mkdir -p $(dir $@)
	@rm -f $@
	@ar rcs $@ $(acl_OBJS)
endif

ifneq ($(acl_HEADERS),)

.PHONY: install_acl_headers
install_acl_headers:
	@mkdir -p $(STAGING_DIR)/include/acl
	@for header in $(acl_HEADERS); do \
		src="include/acl/$$header"; \
		dst="$(STAGING_DIR)/include/acl/$$header"; \
		mkdir -p $$(dirname $$dst); \
		cp -f $$src $$dst; \
	done

# 注册到全局头文件安装目标列表
HEADER_TARGETS += install_acl_headers
endif

endif

CLEAN_TARGETS += $(acl_OBJS) $(acl_SO_TARGET) $(acl_A_TARGET)
