# =============================================================================
# HAL (Hardware Abstraction Layer) 模块构建配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 构建目标配置
# -----------------------------------------------------------------------------
# 根据 Kconfig 配置决定构建静态库或动态库

# 静态库
lib-$(CONFIG_HAL_BUILD_STATIC) += hal

# 动态库
so-$(CONFIG_HAL_BUILD_SHARED) += hal

# -----------------------------------------------------------------------------
# 2. 硬件平台目录选择
# -----------------------------------------------------------------------------
# 根据 Kconfig 配置选择硬件平台实现目录
ifeq ($(CONFIG_HAL_PLATFORM_GENERIC_LINUX),y)
  HAL_PLATFORM_DIR := generic-linux
else ifeq ($(CONFIG_HAL_PLATFORM_RASPBERRY_PI),y)
  HAL_PLATFORM_DIR := raspberry-pi
else ifeq ($(CONFIG_HAL_PLATFORM_BEAGLEBONE),y)
  HAL_PLATFORM_DIR := beaglebone
else ifeq ($(CONFIG_HAL_PLATFORM_CUSTOM),y)
  HAL_PLATFORM_DIR := custom
else
  # 如果没有配置，使用默认值（避免 clean 时出错）
  HAL_PLATFORM_DIR := generic-linux
endif

# -----------------------------------------------------------------------------
# 3. 源文件列表（条件编译）
# -----------------------------------------------------------------------------
# 根据平台和驱动配置选择源文件
hal-objs :=
hal-objs += $(if $(CONFIG_HAL_CAN),src/$(HAL_PLATFORM_DIR)/hal_can_linux.o)
hal-objs += $(if $(CONFIG_HAL_UART),src/$(HAL_PLATFORM_DIR)/hal_serial_linux.o)
hal-objs += $(if $(CONFIG_HAL_I2C),src/$(HAL_PLATFORM_DIR)/hal_i2c_linux.o)
hal-objs += $(if $(CONFIG_HAL_SPI),src/$(HAL_PLATFORM_DIR)/hal_spi_linux.o)
hal-objs += $(if $(CONFIG_HAL_GPIO),src/$(HAL_PLATFORM_DIR)/hal_gpio_linux.o)
hal-objs += $(if $(CONFIG_HAL_WATCHDOG),src/$(HAL_PLATFORM_DIR)/hal_watchdog_linux.o)

# -----------------------------------------------------------------------------
# 4. 编译标志
# -----------------------------------------------------------------------------
# 通用编译标志（引用统一的 include 目录）
ccflags-y += -I$(srctree)/include/hal
ccflags-y += -I$(srctree)/include/hal/config
ccflags-y += -I$(srctree)/include/osal

# 动态库需要 -fPIC
hal-cflags-y := $(if $(CONFIG_HAL_BUILD_SHARED),-fPIC)

# 调试标志
ccflags-$(CONFIG_DEBUG_HAL) += -DDEBUG_HAL

# 特定驱动的编译标志
CFLAGS_hal_can_linux.o := -DHAL_CAN_VERSION=\"1.0\"

# -----------------------------------------------------------------------------
# 5. 链接标志
# -----------------------------------------------------------------------------
# 动态库链接标志
hal-ldflags := $(if $(CONFIG_HAL_BUILD_SHARED),-shared -Wl$(comma)-soname$(comma)libhal.so.1)
hal-ldflags += -L$(LIB_DIR) -losal

# -----------------------------------------------------------------------------
# 6. 清理规则
# -----------------------------------------------------------------------------
clean-files := *.so *.so.* *.a
