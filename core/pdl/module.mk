# =============================================================================
# PDL 模块构建配置（非递归 Make）
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 包含源文件配置
# -----------------------------------------------------------------------------
pdl_SRCS :=
include core/pdl/src/module.mk

# -----------------------------------------------------------------------------
# 2. 编译标志
# -----------------------------------------------------------------------------
pdl_CFLAGS := \
	-Iinclude

# -----------------------------------------------------------------------------
# 3. 链接标志
# -----------------------------------------------------------------------------
pdl_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,-soname,libpdl.so.1 \
	-Wl,--no-as-needed \
	-lpcl \
	-lhal \
	-losal \
	-Wl,--as-needed

# -----------------------------------------------------------------------------
# 4. 导出头文件
# -----------------------------------------------------------------------------
pdl_HEADERS := \
	pdl_watchdog.h \
	pdl_satellite.h \
	pdl_bmc.h \
	pdl_mcu.h

# -----------------------------------------------------------------------------
# 以下为标准构建流程
# -----------------------------------------------------------------------------

pdl_OBJS := $(call srcs_to_objs,$(pdl_SRCS))

ifeq ($(CONFIG_PDL),y)
  ifeq ($(CONFIG_PDL_BUILD_SHARED),y)
    pdl_SO_TARGET := $(STAGING_DIR)/lib/libpdl.so
    ALL_TARGETS += $(pdl_SO_TARGET)
  endif

  ifeq ($(CONFIG_PDL_BUILD_STATIC),y)
    pdl_A_TARGET := $(STAGING_DIR)/lib/libpdl.a
    ALL_TARGETS += $(pdl_A_TARGET)
  endif
endif

$(pdl_OBJS): CFLAGS += $(pdl_CFLAGS)

# 确保 PDL 目标文件在自己的头文件安装后编译
ifneq ($(pdl_HEADERS),)
$(pdl_OBJS): | install_pdl_headers
endif

# 确保在 OSAL/HAL/PCL 头文件安装后才编译 PDL 文件
ifeq ($(CONFIG_OSAL),y)
$(pdl_OBJS): | $(STAGING_DIR)/lib/libosal.so
endif
ifeq ($(CONFIG_HAL),y)
$(pdl_OBJS): | $(STAGING_DIR)/lib/libhal.so
endif
ifeq ($(CONFIG_PCL),y)
$(pdl_OBJS): | $(STAGING_DIR)/lib/libpcl.so
endif

ifeq ($(CONFIG_PDL),y)

ifeq ($(CONFIG_PDL_BUILD_SHARED),y)
$(pdl_SO_TARGET): $(pdl_OBJS)
$(pdl_SO_TARGET): $(STAGING_DIR)/lib/libpcl.so $(STAGING_DIR)/lib/libhal.so $(STAGING_DIR)/lib/libosal.so

$(pdl_SO_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -shared -o $@ $(pdl_OBJS) $(pdl_LDFLAGS)
	@if [ -n "$(pdl_LDFLAGS)" ] && echo "$(pdl_LDFLAGS)" | grep -q "soname,"; then \
		soname=$$(echo "$(pdl_LDFLAGS)" | sed -n 's/.*-soname,\([^ ]*\).*/\1/p'); \
		if [ -n "$$soname" ] && [ "$$soname" != "$$(basename $@)" ]; then \
			ln -sf $$(basename $@) $$(dirname $@)/$$soname; \
		fi; \
	fi
endif

ifeq ($(CONFIG_PDL_BUILD_STATIC),y)
$(pdl_A_TARGET): $(pdl_OBJS)

$(pdl_A_TARGET):
	@echo "  AR      $@"
	@mkdir -p $(dir $@)
	@rm -f $@
	@ar rcs $@ $(pdl_OBJS)
endif

ifneq ($(pdl_HEADERS),)
$(pdl_SO_TARGET) $(pdl_A_TARGET): | install_pdl_headers

.PHONY: install_pdl_headers
install_pdl_headers:
	@mkdir -p $(STAGING_DIR)/include/pdl
	@for header in $(pdl_HEADERS); do \
		src="include/pdl/$$header"; \
		dst="$(STAGING_DIR)/include/pdl/$$header"; \
		mkdir -p $$(dirname $$dst); \
		cp -f $$src $$dst; \
	done
endif

endif

CLEAN_TARGETS += $(pdl_OBJS) $(pdl_SO_TARGET) $(pdl_A_TARGET)
