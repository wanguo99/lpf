# ============================================================
# ACONFIG 库定义
# ============================================================

############## Add source files ###############
# ACONFIG 源文件
list(APPEND ADD_SRCS
    "src/aconfig_api.c"
)

# 添加平台配置源文件（如果有）
if(ACONFIG_EXTRA_SRCS)
    list(APPEND ADD_SRCS ${ACONFIG_EXTRA_SRCS})
endif()
###############################################

################# Add include #################
# 公共头文件目录（对外暴露）
# 用户代码通过 <aconfig/xxx.h> 包含
list(APPEND ADD_INCLUDE "include/aconfig")

# 私有头文件目录（仅内部使用，不传递给依赖者）
list(APPEND ADD_PRIVATE_INCLUDE "src")

# 添加平台配置头文件路径（如果有）
if(ACONFIG_EXTRA_INCLUDE)
    list(APPEND ADD_INCLUDE ${ACONFIG_EXTRA_INCLUDE})
endif()
###############################################

###### Add required/dependent components ######
# ACONFIG 提供通用的应用配置管理框架，只依赖 OSAL 基础服务。

# OSAL 是必须的（Kconfig: depends on OSAL）
if(CONFIG_OSAL)
    list(APPEND ADD_REQUIREMENTS osal)
else()
    message(FATAL_ERROR "ACONFIG requires OSAL but CONFIG_OSAL is not enabled")
endif()
###############################################

#### Add compile option for this component ####
if(CONFIG_ACONFIG_MAX_DEVICES)
    list(APPEND ADD_DEFINITIONS -DACONFIG_MAX_DEVICES=${CONFIG_ACONFIG_MAX_DEVICES})
endif()
###############################################

# 注册组件
if(CONFIG_ACONFIG_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
