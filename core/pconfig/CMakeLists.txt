################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "include")
list(APPEND ADD_INCLUDE "include/pconfig")

# 私有头文件目录（仅内部使用）
list(APPEND ADD_PRIV_INCLUDE "src")

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

# 注册组件
if(CONFIG_PCONFIG_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
