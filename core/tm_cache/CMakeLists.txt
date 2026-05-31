################# Add include #################
# 使用标准的 core 组件 include 路径
###############################################

############## Add source files ###############
# TM_CACHE 源文件
list(APPEND ADD_SRCS
    "src/tm_cache.c"
)
###############################################

###### Add required/dependent components ######
# TM_CACHE 只依赖 OSAL
list(APPEND ADD_REQUIREMENTS osal)
###############################################

#### Add compile option for this component ####
if(CONFIG_TM_CACHE_MAX_ENTRIES)
    list(APPEND ADD_DEFINITIONS -DTM_CACHE_MAX_ENTRIES=${CONFIG_TM_CACHE_MAX_ENTRIES})
endif()
###############################################

# 注册组件（使用公共函数）
list(APPEND ADD_INCLUDE "include")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_TM_CACHE_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
