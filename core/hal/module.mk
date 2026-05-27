# =============================================================================
# HAL 模块构建配置（非递归 Make）
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 确定平台实现目录
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_PLATFORM_GENERIC_LINUX),y)
  HAL_PLATFORM_DIR := generic-linux
else ifeq ($(CONFIG_PLATFORM_TI_AM62X),y)
  HAL_PLATFORM_DIR := ti-am62x
else ifeq ($(CONFIG_PLATFORM_XILINX_ZYNQ),y)
  HAL_PLATFORM_DIR := xilinx-zynq
else
  HAL_PLATFORM_DIR := generic-linux
endif

# -----------------------------------------------------------------------------
# 2. 包含平台相关的源文件配置
# -----------------------------------------------------------------------------
hal_SRCS :=
include core/hal/src/$(HAL_PLATFORM_DIR)/module.mk

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
hal_CFLAGS := \
	-Iinclude

# 平台相关宏定义
ifeq ($(CONFIG_PLATFORM_GENERIC_LINUX),y)
  hal_CFLAGS += -DHAL_PLATFORM_GENERIC_LINUX
endif

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
hal_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,-soname,libhal.so.1 \
	-Wl,--no-as-needed \
	-losal \
	-Wl,--as-needed

# -----------------------------------------------------------------------------
# 5. 导出头文件
# -----------------------------------------------------------------------------
hal_HEADERS := \
	hal_can.h \
	hal_i2c.h \
	hal_spi.h \
	hal_gpio.h \
	hal_watchdog.h \
	hal_serial.h \
	config/can_config.h \
	config/can_types.h \
	config/i2c_types.h \
	config/spi_types.h \
	config/uart_config.h

# -----------------------------------------------------------------------------
# 以下为标准构建流程
# -----------------------------------------------------------------------------

hal_OBJS := $(call srcs_to_objs,$(hal_SRCS))

ifeq ($(CONFIG_HAL),y)
  ifeq ($(CONFIG_HAL_BUILD_SHARED),y)
    hal_SO_TARGET := $(STAGING_DIR)/lib/libhal.so
    ALL_TARGETS += $(hal_SO_TARGET)
  endif

  ifeq ($(CONFIG_HAL_BUILD_STATIC),y)
    hal_A_TARGET := $(STAGING_DIR)/lib/libhal.a
    ALL_TARGETS += $(hal_A_TARGET)
  endif
endif

$(hal_OBJS): CFLAGS += $(hal_CFLAGS)



ifeq ($(CONFIG_HAL),y)

ifeq ($(CONFIG_HAL_BUILD_SHARED),y)
$(hal_SO_TARGET): $(hal_OBJS)
$(hal_SO_TARGET): $(STAGING_DIR)/lib/libosal.so

$(hal_SO_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -shared -o $@ $(hal_OBJS) $(hal_LDFLAGS)
	@if [ -n "$(hal_LDFLAGS)" ] && echo "$(hal_LDFLAGS)" | grep -q "soname,"; then \
		soname=$$(echo "$(hal_LDFLAGS)" | sed -n 's/.*-soname,\([^ ]*\).*/\1/p'); \
		if [ -n "$$soname" ] && [ "$$soname" != "$$(basename $@)" ]; then \
			ln -sf $$(basename $@) $$(dirname $@)/$$soname; \
		fi; \
	fi
endif

ifeq ($(CONFIG_HAL_BUILD_STATIC),y)
$(hal_A_TARGET): $(hal_OBJS)

$(hal_A_TARGET):
	@echo "  AR      $@"
	@mkdir -p $(dir $@)
	@rm -f $@
	@ar rcs $@ $(hal_OBJS)
endif

ifneq ($(hal_HEADERS),)
$(hal_SO_TARGET) $(hal_A_TARGET): | install_hal_headers

.PHONY: install_hal_headers
install_hal_headers:
	@mkdir -p $(STAGING_DIR)/include/hal
	@for header in $(hal_HEADERS); do \
		src="include/hal/$$header"; \
		dst="$(STAGING_DIR)/include/hal/$$header"; \
		mkdir -p $$(dirname $$dst); \
		cp -f $$src $$dst; \
	done

# 注册到全局头文件安装目标列表
HEADER_TARGETS += install_hal_headers
endif

endif

CLEAN_TARGETS += $(hal_OBJS) $(hal_SO_TARGET) $(hal_A_TARGET)
