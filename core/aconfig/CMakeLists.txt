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
list(APPEND ADD_PRIV_INCLUDE "src")

# 添加平台配置头文件路径（如果有）
if(ACONFIG_EXTRA_INCLUDE)
    list(APPEND ADD_INCLUDE ${ACONFIG_EXTRA_INCLUDE})
endif()
###############################################

###### Add required/dependent components ######
# ACONFIG 只依赖 OSAL
# ACONFIG 提供通用的遥测遥控配置管理框架
# 不依赖具体的设备驱动层（PDL）
list(APPEND ADD_REQUIREMENTS osal)
###############################################

#### Add compile option for this component ####
if(CONFIG_ACL_MAX_DEVICES)
    list(APPEND ADD_DEFINITIONS -DACONFIG_MAX_DEVICES=${CONFIG_ACL_MAX_DEVICES})
endif()
###############################################

# 注册组件
if(CONFIG_ACL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
