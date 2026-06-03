################# Add include #################
# 使用标准的 core 组件 include 路径
# 添加平台配置头文件路径（如果有）
if(ACONFIG_EXTRA_INCLUDE)
    list(APPEND ADD_INCLUDE ${ACONFIG_EXTRA_INCLUDE})
endif()
###############################################

############## Add source files ###############
# ACL 源文件
list(APPEND ADD_SRCS
    "src/aconfig_api.c"
)

# 添加平台配置源文件（如果有）
if(ACONFIG_EXTRA_SRCS)
    list(APPEND ADD_SRCS ${ACONFIG_EXTRA_SRCS})
endif()
###############################################

###### Add required/dependent components ######
# ACL 只依赖 OSAL
# ACL 提供通用的遥测遥控配置管理框架
# 不依赖具体的设备驱动层（PDL）
list(APPEND ADD_REQUIREMENTS osal)
###############################################

#### Add compile option for this component ####
if(CONFIG_ACL_MAX_DEVICES)
    list(APPEND ADD_DEFINITIONS -DACL_MAX_DEVICES=${CONFIG_ACL_MAX_DEVICES})
endif()
###############################################

# 注册组件（使用公共函数）
# api 目录：对外 API 头文件（PUBLIC）
# include 目录：内部实现头文件（PRIVATE，保留用于向后兼容）
list(APPEND ADD_INCLUDE "api")
list(APPEND ADD_INCLUDE "include")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_ACL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
