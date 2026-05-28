################# Add include #################
# 使用标准的 core 组件 include 路径
# 添加平台配置头文件路径（如果有）
if(ACL_EXTRA_INCLUDE)
    list(APPEND ADD_INCLUDE ${ACL_EXTRA_INCLUDE})
endif()
###############################################

############## Add source files ###############
# ACL 源文件
list(APPEND ADD_SRCS
    "src/acl_api.c"
    "src/acl_api_v2.c"
    "src/acl_telemetry_cache.c"
)

# 添加平台配置源文件（如果有）
if(ACL_EXTRA_SRCS)
    list(APPEND ADD_SRCS ${ACL_EXTRA_SRCS})
endif()
###############################################

###### Add required/dependent components ######
# ACL 依赖 OSAL 和 PDL
list(APPEND ADD_REQUIREMENTS osal pdl)
###############################################

#### Add compile option for this component ####
if(CONFIG_ACL_MAX_DEVICES)
    list(APPEND ADD_DEFINITIONS -DACL_MAX_DEVICES=${CONFIG_ACL_MAX_DEVICES})
endif()
###############################################

# 注册组件（使用公共函数）
list(APPEND ADD_INCLUDE "include")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_ACL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
