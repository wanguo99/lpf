################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "include")

# 私有头文件目录（仅内部使用）
list(APPEND ADD_PRIVATE_INCLUDE "src")
list(APPEND ADD_PRIVATE_INCLUDE "src/include")

# Linux 平台子目录
if(CONFIG_PLATFORM_LINUX)
    list(APPEND ADD_PRIVATE_INCLUDE "src/linux/can")
    list(APPEND ADD_PRIVATE_INCLUDE "src/linux/gpio")
    list(APPEND ADD_PRIVATE_INCLUDE "src/linux/i2c")
    list(APPEND ADD_PRIVATE_INCLUDE "src/linux/spi")
    list(APPEND ADD_PRIVATE_INCLUDE "src/linux/serial")
    list(APPEND ADD_PRIVATE_INCLUDE "src/linux/watchdog")
endif()

# macOS 平台子目录
if(CONFIG_OS_MACOS)
    list(APPEND ADD_PRIVATE_INCLUDE "src/macos/can")
    list(APPEND ADD_PRIVATE_INCLUDE "src/macos/gpio")
    list(APPEND ADD_PRIVATE_INCLUDE "src/macos/i2c")
    list(APPEND ADD_PRIVATE_INCLUDE "src/macos/spi")
    list(APPEND ADD_PRIVATE_INCLUDE "src/macos/serial")
    list(APPEND ADD_PRIVATE_INCLUDE "src/macos/watchdog")
endif()
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
list(APPEND ADD_INCLUDE "include")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_HAL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
