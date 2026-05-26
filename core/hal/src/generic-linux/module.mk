# =============================================================================
# HAL Generic Linux 平台源文件配置
# =============================================================================

# CAN 驱动
ifeq ($(CONFIG_HAL_CAN),y)
hal_SRCS += core/hal/src/generic-linux/hal_can_linux.c
endif

# UART 驱动
ifeq ($(CONFIG_HAL_UART),y)
hal_SRCS += core/hal/src/generic-linux/hal_serial_linux.c
endif

# I2C 驱动
ifeq ($(CONFIG_HAL_I2C),y)
hal_SRCS += core/hal/src/generic-linux/hal_i2c_linux.c
endif

# SPI 驱动
ifeq ($(CONFIG_HAL_SPI),y)
hal_SRCS += core/hal/src/generic-linux/hal_spi_linux.c
endif

# GPIO 驱动
ifeq ($(CONFIG_HAL_GPIO),y)
hal_SRCS += core/hal/src/generic-linux/hal_gpio_linux.c
endif

# Watchdog 驱动
ifeq ($(CONFIG_HAL_WATCHDOG),y)
hal_SRCS += core/hal/src/generic-linux/hal_watchdog_linux.c
endif
