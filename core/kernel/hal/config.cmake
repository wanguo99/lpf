# =============================================================================
# HAL 模块配置
# =============================================================================
# 功能：根据 Kconfig 配置选择需要编译的源文件
# 用法：include(config.cmake)
#       然后使用 ${HAL_SRCS} 变量
# =============================================================================

message(STATUS "Configuring HAL module...")

# 初始化源文件列表
set(HAL_SRCS "")

# 根据 Kconfig 选择性添加驱动源文件
if(CONFIG_HAL_CAN)
    list(APPEND HAL_SRCS "src/${HAL_PLATFORM_DIR}/hal_can_linux.c")
    message(STATUS "  [HAL] CAN driver enabled")
endif()

if(CONFIG_HAL_UART)
    list(APPEND HAL_SRCS "src/${HAL_PLATFORM_DIR}/hal_serial_linux.c")
    message(STATUS "  [HAL] UART driver enabled")
endif()

if(CONFIG_HAL_I2C)
    list(APPEND HAL_SRCS "src/${HAL_PLATFORM_DIR}/hal_i2c_linux.c")
    message(STATUS "  [HAL] I2C driver enabled")
endif()

if(CONFIG_HAL_SPI)
    list(APPEND HAL_SRCS "src/${HAL_PLATFORM_DIR}/hal_spi_linux.c")
    message(STATUS "  [HAL] SPI driver enabled")
endif()

if(CONFIG_HAL_GPIO)
    list(APPEND HAL_SRCS "src/${HAL_PLATFORM_DIR}/hal_gpio_linux.c")
    message(STATUS "  [HAL] GPIO driver enabled")
endif()

# 验证至少有一个驱动被启用
if(NOT HAL_SRCS)
    message(FATAL_ERROR "HAL: No drivers enabled. Please enable at least one driver in menuconfig.")
endif()

message(STATUS "  [HAL] Total ${CMAKE_MATCH_COUNT} drivers enabled")
