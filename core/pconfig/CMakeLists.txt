################# Add include #################
# 使用标准的 core 组件 include 路径
# 注意：不再直接包含 PDL 头文件，而是通过 types 模块获取配置类型定义
# 这样可以打破 PCL 和 PDL 之间的循环依赖

# 添加平台配置头文件路径（如果有）
if(PCONFIG_EXTRA_INCLUDE)
    list(APPEND ADD_INCLUDE ${PCONFIG_EXTRA_INCLUDE})
endif()
###############################################

############## Add source files ###############
# PCL 核心源文件
list(APPEND ADD_SRCS
    "src/pconfig_api.c"
    "src/pconfig_register.c"
)

# 添加平台配置源文件（如果有）
if(PCONFIG_EXTRA_SRCS)
    list(APPEND ADD_SRCS ${PCONFIG_EXTRA_SRCS})
endif()
###############################################

###### Add required/dependent components ######
# PCL 依赖 OSAL 和 PDL
# PCL 使用 PDL 的配置类型定义（pdl_types.h）
# 依赖关系：PCL→PDL（头文件），PDL→PCL（配置数据）
# 这不是循环依赖，因为是不同层面的依赖
list(APPEND ADD_REQUIREMENTS osal pdl)
###############################################

#### Add compile option for this component ####
# 平台配置通过 CONFIG_PROJECT_NAME 由产品层提供
###############################################

# 注册组件（使用公共函数）
# api/ 目录对外暴露，include/ 目录仅内部使用
list(APPEND ADD_INCLUDE "api" "include")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_PCONFIG_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
