# ============================================================================
# HAL 模块构建 - Kbuild 风格
# ============================================================================

# 包含 Kconfig 配置
-include $(objtree)/include/config/auto.conf

# 库名称
LIB_NAME := hal

# 根据 Kconfig 配置决定构建目标
ifeq ($(CONFIG_HAL_BUILD_STATIC),y)
  obj-s := libhal.a
endif

ifeq ($(CONFIG_HAL_BUILD_SHARED),y)
  obj-d := libhal.so
endif

# 对象文件列表（Linux 内核风格）
# Linux 平台驱动
libhal.a-objs-y :=
libhal.a-objs-$(CONFIG_HAL_CAN) += src/linux/hal_can_linux.o
libhal.a-objs-$(CONFIG_HAL_UART) += src/linux/hal_serial_linux.o
libhal.a-objs-$(CONFIG_HAL_I2C) += src/linux/hal_i2c_linux.o
libhal.a-objs-$(CONFIG_HAL_SPI) += src/linux/hal_spi_linux.o
libhal.a-objs-$(CONFIG_HAL_GPIO) += src/linux/hal_gpio_linux.o
libhal.a-objs-$(CONFIG_HAL_WATCHDOG) += src/linux/hal_watchdog_linux.o

# 动态库使用相同的对象文件
libhal.so-objs-y := $(libhal.a-objs-y)
libhal.so-objs-$(CONFIG_HAL_CAN) := $(libhal.a-objs-y)
libhal.so-objs-$(CONFIG_HAL_UART) := $(libhal.a-objs-y)
libhal.so-objs-$(CONFIG_HAL_I2C) := $(libhal.a-objs-y)
libhal.so-objs-$(CONFIG_HAL_SPI) := $(libhal.a-objs-y)
libhal.so-objs-$(CONFIG_HAL_GPIO) := $(libhal.a-objs-y)
libhal.so-objs-$(CONFIG_HAL_WATCHDOG) := $(libhal.a-objs-y)

# 私有编译标志（每个模块独立定义）
CFLAGS_PRIV := -I$(srctree)/core/hal/include
CFLAGS_PRIV += -I$(srctree)/core/hal/include/config
CFLAGS_PRIV += -I$(srctree)/core/osal/include
CFLAGS_PRIV += -I$(srctree)/include

# 私有链接标志和库（每个模块独立定义）
LDFLAGS_PRIV :=
LDLIBS_PRIV := -L$(objtree)/lib -losal

# 导出的头文件
EXPORT_HEADERS := \
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

# 包含统一构建框架
include $(scriptdir)/build/Makefile.build
