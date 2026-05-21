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
# 2. 源文件列表（条件编译）
# -----------------------------------------------------------------------------
# Linux 平台驱动
hal-objs :=
hal-objs += $(if $(CONFIG_HAL_CAN),src/linux/hal_can_linux.o)
hal-objs += $(if $(CONFIG_HAL_UART),src/linux/hal_serial_linux.o)
hal-objs += $(if $(CONFIG_HAL_I2C),src/linux/hal_i2c_linux.o)
hal-objs += $(if $(CONFIG_HAL_SPI),src/linux/hal_spi_linux.o)
hal-objs += $(if $(CONFIG_HAL_GPIO),src/linux/hal_gpio_linux.o)
hal-objs += $(if $(CONFIG_HAL_WATCHDOG),src/linux/hal_watchdog_linux.o)

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
# 通用编译标志
ccflags-y += -I$(src)/include
ccflags-y += -I$(src)/include/config
ccflags-y += -I$(srctree)/core/osal/include
ccflags-y += -I$(srctree)/include

# 动态库需要 -fPIC
hal-cflags-y := $(if $(CONFIG_HAL_BUILD_SHARED),-fPIC)

# 调试标志
ccflags-$(CONFIG_DEBUG_HAL) += -DDEBUG_HAL

# 特定驱动的编译标志
CFLAGS_hal_can_linux.o := -DHAL_CAN_VERSION=\"1.0\"

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
# 动态库链接标志
hal-ldflags := $(if $(CONFIG_HAL_BUILD_SHARED),-shared -Wl$(comma)-soname$(comma)libhal.so.1)
hal-ldflags += -L$(objtree)/lib -losal

# -----------------------------------------------------------------------------
# 5. 头文件导出
# -----------------------------------------------------------------------------
header-y := \
	hal_can.h \
	hal_serial.h \
	hal_i2c.h \
	hal_spi.h \
	hal_gpio.h \
	hal_watchdog.h \
	config/hal_can_config.h \
	config/hal_serial_config.h \
	config/hal_i2c_config.h \
	config/hal_spi_config.h \
	config/hal_gpio_config.h \
	config/hal_watchdog_config.h

# -----------------------------------------------------------------------------
# 6. 清理规则
# -----------------------------------------------------------------------------
clean-files := *.so *.so.* *.a
