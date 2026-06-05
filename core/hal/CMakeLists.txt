################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "include")

###############################################

############## Add source files ###############
# 根据 Kconfig 配置精确添加源文件

# ============================================================================
# Platform-specific driver source files
# ============================================================================

if(CONFIG_PLATFORM_LINUX)
    # Linux platform drivers

    if(CONFIG_HAL_CAN_LINUX)
        list(APPEND ADD_SRCS "src/linux/can/hal_can_linux.c")
    endif()

    if(CONFIG_HAL_GPIO_LINUX)
        list(APPEND ADD_SRCS "src/linux/gpio/hal_gpio_linux.c")
    endif()

    if(CONFIG_HAL_I2C_LINUX)
        list(APPEND ADD_SRCS "src/linux/i2c/hal_i2c_linux.c")
    endif()

    if(CONFIG_HAL_SPI_LINUX)
        list(APPEND ADD_SRCS "src/linux/spi/hal_spi_linux.c")
    endif()

    if(CONFIG_HAL_UART_LINUX)
        list(APPEND ADD_SRCS "src/linux/serial/hal_serial_linux.c")
    endif()

    if(CONFIG_HAL_WATCHDOG_LINUX)
        list(APPEND ADD_SRCS "src/linux/watchdog/hal_watchdog_linux.c")
    endif()

elseif(CONFIG_OS_MACOS)
    # macOS platform drivers

    if(CONFIG_HAL_CAN_MACOS)
        list(APPEND ADD_SRCS "src/macos/can/hal_can_macos.c")
    endif()

    if(CONFIG_HAL_GPIO_MACOS)
        list(APPEND ADD_SRCS "src/macos/gpio/hal_gpio_macos.c")
    endif()

    if(CONFIG_HAL_I2C_MACOS)
        list(APPEND ADD_SRCS "src/macos/i2c/hal_i2c_macos.c")
    endif()

    if(CONFIG_HAL_SPI_MACOS)
        list(APPEND ADD_SRCS "src/macos/spi/hal_spi_macos.c")
    endif()

    if(CONFIG_HAL_UART_MACOS)
        list(APPEND ADD_SRCS "src/macos/serial/hal_serial_macos.c")
    endif()

    if(CONFIG_HAL_WATCHDOG_MACOS)
        list(APPEND ADD_SRCS "src/macos/watchdog/hal_watchdog_macos.c")
    endif()

else()
    message(WARNING "No HAL platform selected or platform not supported")
endif()

# Check if any source files were added
list(LENGTH ADD_SRCS SRCS_COUNT)
if(SRCS_COUNT EQUAL 0)
    message(FATAL_ERROR "No HAL driver source files selected. Please check your Kconfig configuration.")
endif()

message(STATUS "HAL: Selected ${SRCS_COUNT} driver source files for compilation")
###############################################

###### Add required/dependent components ######
# HAL 依赖 OSAL
list(APPEND ADD_REQUIREMENTS osal)
###############################################

#### Add compile option for this component ####
if(CONFIG_HAL_DEBUG)
    list(APPEND ADD_DEFINITIONS -DHAL_DEBUG=1)
endif()

if(CONFIG_HAL_STATISTICS)
    list(APPEND ADD_DEFINITIONS -DHAL_STATISTICS=1)
endif()

if(CONFIG_HAL_POWER_MANAGEMENT)
    list(APPEND ADD_DEFINITIONS -DHAL_POWER_MANAGEMENT=1)
endif()
###############################################

# 注册组件（使用公共函数）
# PUBLIC: include/ 目录（对外API）
list(APPEND ADD_INCLUDE "include/hal")
list(APPEND ADD_INCLUDE "include/hal/config")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_HAL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
