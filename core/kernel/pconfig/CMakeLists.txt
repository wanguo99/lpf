################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "${SDK_PATH}/core/kernel/include/pconfig")

# 私有头文件目录（仅内部使用）
list(APPEND ADD_PRIVATE_INCLUDE "src")

# 添加平台配置头文件路径（如果有）
if(PCONFIG_EXTRA_INCLUDE)
    list(APPEND ADD_INCLUDE ${PCONFIG_EXTRA_INCLUDE})
endif()
###############################################

############## Add source files ###############
# PCONFIG 核心源文件
list(APPEND ADD_SRCS
    "src/pconfig_api.c"
)

# 添加平台配置源文件（如果有）
if(PCONFIG_EXTRA_SRCS)
    list(APPEND ADD_SRCS ${PCONFIG_EXTRA_SRCS})
endif()
###############################################

###### Add required/dependent components ######
# PCONFIG 定义硬件配置类型并提供只读查询，业务驱动层依赖它。
# 依赖关系保持单向：PDL/产品代码 -> PCONFIG -> OSAL。

# OSAL 是必须的（Kconfig: depends on OSAL）
if(CONFIG_OSAL)
    list(APPEND ADD_REQUIREMENTS osal)
else()
    message(FATAL_ERROR "PCONFIG requires OSAL but CONFIG_OSAL is not enabled")
endif()
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
