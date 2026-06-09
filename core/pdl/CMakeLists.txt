################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "include")

# 私有头文件目录（仅内部使用）
list(APPEND ADD_PRIVATE_INCLUDE "src")
###############################################

############## Add source files ###############
# 根据配置添加子模块源文件
if(CONFIG_PDL_SATELLITE_SUPPORT)
    file(GLOB PDL_SATELLITE_SRCS "src/pdl_satellite/*.c")
    list(APPEND ADD_SRCS ${PDL_SATELLITE_SRCS})
endif()

if(CONFIG_PDL_BMC_SUPPORT)
    file(GLOB PDL_BMC_SRCS "src/pdl_bmc/*.c")
    list(APPEND ADD_SRCS ${PDL_BMC_SRCS})
endif()

if(CONFIG_PDL_MCU_SUPPORT)
    file(GLOB PDL_MCU_SRCS "src/pdl_mcu/*.c")
    list(APPEND ADD_SRCS ${PDL_MCU_SRCS})
endif()

if(CONFIG_PDL_WATCHDOG_SUPPORT)
    file(GLOB PDL_WATCHDOG_SRCS "src/pdl_watchdog/*.c")
    list(APPEND ADD_SRCS ${PDL_WATCHDOG_SRCS})
endif()

if(CONFIG_PDL_CCM_SUPPORT)
    file(GLOB PDL_CCM_SRCS "src/pdl_ccm/*.c")
    list(APPEND ADD_SRCS ${PDL_CCM_SRCS})
endif()

if(CONFIG_PDL_MISC_SUPPORT)
    file(GLOB PDL_MISC_SRCS "src/pdl_misc/*.c")
    list(APPEND ADD_SRCS ${PDL_MISC_SRCS})
endif()
###############################################

###### Add required/dependent components ######
# PDL 依赖 OSAL 和 HAL
# PDL 定义配置类型（pdl_types.h），不依赖 PConfig
# PConfig 使用 PDL 的配置类型（包含 pdl_types.h）
# 依赖关系：PConfig→PDL（头文件），无循环依赖
# PDL_CCM 和 PDL_MCU 需要 PRL 协议层支持
list(APPEND ADD_REQUIREMENTS osal hal)

if(CONFIG_PDL_CCM_SUPPORT OR CONFIG_PDL_MCU_SUPPORT)
    list(APPEND ADD_REQUIREMENTS prl)
endif()
###############################################

#### Add compile option for this component ####
if(CONFIG_PDL_SATELLITE_SUPPORT)
    list(APPEND ADD_DEFINITIONS -DPDL_SATELLITE_SUPPORT=1)
endif()

if(CONFIG_PDL_BMC_SUPPORT)
    list(APPEND ADD_DEFINITIONS -DPDL_BMC_SUPPORT=1)
endif()

if(CONFIG_PDL_MCU_SUPPORT)
    list(APPEND ADD_DEFINITIONS -DPDL_MCU_SUPPORT=1)

    if(CONFIG_PDL_MCU_CAN_SUPPORT)
        list(APPEND ADD_DEFINITIONS -DPDL_MCU_CAN_SUPPORT=1)
    endif()

    if(CONFIG_PDL_MCU_UART_SUPPORT)
        list(APPEND ADD_DEFINITIONS -DPDL_MCU_UART_SUPPORT=1)
    endif()
endif()

if(CONFIG_PDL_WATCHDOG_SUPPORT)
    list(APPEND ADD_DEFINITIONS -DPDL_WATCHDOG_SUPPORT=1)
endif()

if(CONFIG_PDL_CCM_SUPPORT)
    list(APPEND ADD_DEFINITIONS -DPDL_CCM_SUPPORT=1)
endif()

if(CONFIG_PDL_MISC_SUPPORT)
    list(APPEND ADD_DEFINITIONS -DPDL_MISC_SUPPORT=1)
endif()
###############################################

# 注册组件（使用公共函数）
list(APPEND ADD_INCLUDE "include/pdl")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_PDL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
