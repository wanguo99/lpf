# =============================================================================
# HAL 模块非递归 Make 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 确定平台目录
# -----------------------------------------------------------------------------
HAL_PLATFORM_DIR := $(CONFIG_HAL_PLATFORM_NAME)

# -----------------------------------------------------------------------------
# 2. 基础源文件（所有平台都需要）
# -----------------------------------------------------------------------------
hal_SRCS :=

# -----------------------------------------------------------------------------
# 3. 条件编译（根据 Kconfig 配置）
# -----------------------------------------------------------------------------

# CAN 支持
ifeq ($(CONFIG_HAL_CAN),y)
hal_SRCS += core/hal/src/$(HAL_PLATFORM_DIR)/hal_can_linux.c
endif

# UART 支持
ifeq ($(CONFIG_HAL_UART),y)
hal_SRCS += core/hal/src/$(HAL_PLATFORM_DIR)/hal_serial_linux.c
endif

# I2C 支持
ifeq ($(CONFIG_HAL_I2C),y)
hal_SRCS += core/hal/src/$(HAL_PLATFORM_DIR)/hal_i2c_linux.c
endif

# SPI 支持
ifeq ($(CONFIG_HAL_SPI),y)
hal_SRCS += core/hal/src/$(HAL_PLATFORM_DIR)/hal_spi_linux.c
endif

# GPIO 支持
ifeq ($(CONFIG_HAL_GPIO),y)
hal_SRCS += core/hal/src/$(HAL_PLATFORM_DIR)/hal_gpio_linux.c
endif

# Watchdog 支持
ifeq ($(CONFIG_HAL_WATCHDOG),y)
hal_SRCS += core/hal/src/$(HAL_PLATFORM_DIR)/hal_watchdog_linux.c
endif

# -----------------------------------------------------------------------------
# 4. 编译标志
# -----------------------------------------------------------------------------
hal_CFLAGS := \
	-Iinclude/hal \
	-Iinclude/hal/config \
	-Iinclude/osal

# -----------------------------------------------------------------------------
# 5. 链接标志
# -----------------------------------------------------------------------------
hal_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-losal \
	-Wl,--as-needed \
	-lpthread

# -----------------------------------------------------------------------------
# 6. 生成目标文件列表
# -----------------------------------------------------------------------------
hal_OBJS := $(call srcs_to_objs,$(hal_SRCS))

# -----------------------------------------------------------------------------
# 7. 定义目标
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_HAL_BUILD_SHARED),y)
hal_TARGET := $(STAGING_DIR)/lib/libhal.so
ALL_TARGETS += $(hal_TARGET)
endif

ifeq ($(CONFIG_HAL_BUILD_STATIC),y)
hal_STATIC_TARGET := $(STAGING_DIR)/lib/libhal.a
ALL_TARGETS += $(hal_STATIC_TARGET)
endif

# -----------------------------------------------------------------------------
# 8. 添加编译标志到全局
# -----------------------------------------------------------------------------
$(hal_OBJS): CFLAGS += $(hal_CFLAGS)

# -----------------------------------------------------------------------------
# 9. 定义构建规则
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_HAL_BUILD_SHARED),y)
$(eval $(call build_shared_lib,$(hal_TARGET),$(hal_OBJS),$(hal_LDFLAGS)))
endif

ifeq ($(CONFIG_HAL_BUILD_STATIC),y)
$(eval $(call build_static_lib,$(hal_STATIC_TARGET),$(hal_OBJS)))
endif

# -----------------------------------------------------------------------------
# 10. 依赖关系
# -----------------------------------------------------------------------------
# hal 依赖 osal
$(hal_TARGET): $(STAGING_DIR)/lib/libosal.so
$(hal_STATIC_TARGET): $(STAGING_DIR)/lib/libosal.so

# -----------------------------------------------------------------------------
# 11. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(hal_OBJS) $(hal_TARGET) $(hal_STATIC_TARGET)
