################# Add include #################
# 使用标准的 core 组件 include 路径
###############################################

############## Add source files ###############
# PDL 核心源文件
list(APPEND ADD_SRCS "src/pdl_watchdog.c")

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
###############################################

###### Add required/dependent components ######
# PDL 依赖 OSAL, HAL, PCL
list(APPEND ADD_REQUIREMENTS osal hal pcl)
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
endif()

if(CONFIG_PDL_WATCHDOG_SUPPORT)
    list(APPEND ADD_DEFINITIONS -DPDL_WATCHDOG_SUPPORT=1)
endif()
###############################################

# 注册组件（使用公共函数）
list(APPEND ADD_INCLUDE "include")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_PDL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
