################# Add include #################
list(APPEND ADD_INCLUDE "include"
    )
# 兼容 #include "acl/xxx.h" 路径
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")
###############################################

############## Add source files ###############
# ACL 源文件
list(APPEND ADD_SRCS
    "src/acl_api.c"
    "src/acl_api_v2.c"
    "src/acl_telemetry_cache.c"
)
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

# 注册组件
if(CONFIG_ACL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
