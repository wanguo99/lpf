################# Add include #################
list(APPEND ADD_INCLUDE "include"
    )
# 兼容 #include "hal/xxx.h" 路径
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")
###############################################

############## Add source files ###############
# 根据平台配置选择源文件
if(CONFIG_PLATFORM_GENERIC_LINUX)
    file(GLOB HAL_SRCS "src/generic-linux/*.c")
    list(APPEND ADD_SRCS ${HAL_SRCS})
elseif(CONFIG_OS_MACOS)
    file(GLOB HAL_SRCS "src/macos/*.c")
    list(APPEND ADD_SRCS ${HAL_SRCS})
endif()
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

# 注册组件
if(CONFIG_HAL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
