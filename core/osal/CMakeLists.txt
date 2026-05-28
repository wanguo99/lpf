################# Add include #################
list(APPEND ADD_INCLUDE "include"
    )
# 为了兼容旧的 #include "osal/xxx.h" 路径
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")
###############################################

############## Add source files ###############
# 根据 Kconfig 配置选择不同平台的源文件
if(CONFIG_OS_LINUX OR CONFIG_OS_MACOS)
    # POSIX 平台源文件
    file(GLOB_RECURSE POSIX_SRCS "src/posix/*.c")
    list(APPEND ADD_SRCS ${POSIX_SRCS})
elseif(CONFIG_OS_WINDOWS)
    # Windows 平台源文件
    file(GLOB_RECURSE WIN32_SRCS "src/win32/*.c")
    list(APPEND ADD_SRCS ${WIN32_SRCS})
elseif(CONFIG_OS_RTOS)
    # RTOS 平台源文件
    file(GLOB_RECURSE RTOS_SRCS "src/rtos/*.c")
    list(APPEND ADD_SRCS ${RTOS_SRCS})
elseif(CONFIG_OS_BARE)
    # Bare metal 平台源文件
    file(GLOB_RECURSE BARE_SRCS "src/bare/*.c")
    list(APPEND ADD_SRCS ${BARE_SRCS})
endif()

# 通用源文件
file(GLOB COMMON_SRCS "src/common/*.c")
list(APPEND ADD_SRCS ${COMMON_SRCS})
###############################################

###### Add required/dependent components ######
# OSAL 是基础层，不依赖其他组件
# 但需要系统库
if(CONFIG_OS_LINUX OR CONFIG_OS_MACOS)
    list(APPEND ADD_REQUIREMENTS pthread rt m)
endif()
###############################################

#### Add compile option for this component ####
# 根据配置添加编译选项
if(CONFIG_OSAL_DEBUG_LOGGING)
    list(APPEND ADD_DEFINITIONS -DOSAL_DEBUG_LOGGING=1)
endif()

# 添加 POSIX 特性宏
if(CONFIG_OS_LINUX OR CONFIG_OS_MACOS)
    list(APPEND ADD_DEFINITIONS
        -D_POSIX_C_SOURCE=200809L
        -D_XOPEN_SOURCE=700
        -D_GNU_SOURCE
    )
endif()
###############################################

# 注册组件
if(CONFIG_OSAL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
