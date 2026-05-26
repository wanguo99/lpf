# =============================================================================
# HAL Generic Linux 平台源文件配置
# =============================================================================

# 定义当前平台目录
hal_platform_dir := core/hal/src/generic-linux

# CAN 驱动（细粒度控制）
ifeq ($(CONFIG_HAL_CAN_LINUX),y)
hal_SRCS += $(hal_platform_dir)/hal_can_linux.c
endif

# UART 驱动（细粒度控制）
ifeq ($(CONFIG_HAL_UART_LINUX),y)
hal_SRCS += $(hal_platform_dir)/hal_serial_linux.c
endif

# I2C 驱动（细粒度控制）
ifeq ($(CONFIG_HAL_I2C_LINUX),y)
hal_SRCS += $(hal_platform_dir)/hal_i2c_linux.c
endif

# SPI 驱动（细粒度控制）
ifeq ($(CONFIG_HAL_SPI_LINUX),y)
hal_SRCS += $(hal_platform_dir)/hal_spi_linux.c
endif

# GPIO 驱动（细粒度控制）
ifeq ($(CONFIG_HAL_GPIO_LINUX),y)
hal_SRCS += $(hal_platform_dir)/hal_gpio_linux.c
endif

# Watchdog 驱动（细粒度控制）
ifeq ($(CONFIG_HAL_WATCHDOG_LINUX),y)
hal_SRCS += $(hal_platform_dir)/hal_watchdog_linux.c
endif
