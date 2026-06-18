################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "${SDK_PATH}/core/kernel/include/hal")
list(APPEND ADD_INCLUDE "${SDK_PATH}/core/kernel/include/hal/config")

###############################################

############## Add source files ###############
# 根据 Kconfig 配置精确添加源文件

# ============================================================================
# Platform-specific driver source files
# ============================================================================

if(CONFIG_PLATFORM_LINUX)
    # Linux user-space drivers

    if(CONFIG_HAL_CAN_LINUX)
        list(APPEND ADD_SRCS "src/can/hal_can_linux.c")
    endif()

    if(CONFIG_HAL_GPIO_LINUX)
        list(APPEND ADD_SRCS "src/gpio/hal_gpio_linux.c")
    endif()

    if(CONFIG_HAL_I2C_LINUX)
        list(APPEND ADD_SRCS "src/i2c/hal_i2c_linux.c")
    endif()

    if(CONFIG_HAL_SPI_LINUX)
        list(APPEND ADD_SRCS "src/spi/hal_spi_linux.c")
    endif()

    if(CONFIG_HAL_UART_LINUX)
        list(APPEND ADD_SRCS "src/serial/hal_serial_linux.c")
    endif()

else()
    message(WARNING "No HAL platform selected or platform not supported")
endif()

# Check if any source files were added
list(LENGTH ADD_SRCS SRCS_COUNT)
if(SRCS_COUNT EQUAL 0)
    message(FATAL_ERROR "No HAL driver source files selected. Please check your Config.in configuration.")
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
list(APPEND ADD_INCLUDE "${SDK_PATH}/core/kernel/include/hal")
list(APPEND ADD_INCLUDE "${SDK_PATH}/core/kernel/include/hal/config")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_HAL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
