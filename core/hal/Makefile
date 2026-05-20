# ============================================================================
# HAL 模块构建 - 使用通用库构建框架（支持条件编译）
# ============================================================================

# 库配置
LIB_NAME := hal
# LIB_TYPE := static  # 取消注释可覆盖全局配置

# 源文件目录（Linux 实现）
SRC_DIR := $(srctree)/core/$(LIB_NAME)/src/linux

# 条件编译：根据 Kconfig 配置选择源文件
SRCS-$(CONFIG_HAL_CAN) += hal_can_linux.c
SRCS-$(CONFIG_HAL_UART) += hal_serial_linux.c
SRCS-$(CONFIG_HAL_I2C) += hal_i2c_linux.c
SRCS-$(CONFIG_HAL_SPI) += hal_spi_linux.c
SRCS-$(CONFIG_HAL_GPIO) += hal_gpio_linux.c
SRCS-$(CONFIG_HAL_WATCHDOG) += hal_watchdog_linux.c

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

# 模块依赖（链接 OSAL）
LDLIBS := -L$(objtree)/lib -losal

# 包含通用库构建框架
include $(srctree)/scripts/Makefile.lib
