################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "include/prl")

# 私有头文件目录（仅内部使用）
list(APPEND ADD_PRIVATE_INCLUDE "src")
###############################################

############## Add source files ###############
# PRL 核心源文件
list(APPEND ADD_SRCS
    "src/prl_common.c"
    "src/prl_device.c"
    "src/prl_api.c"
)

# 设备特定协议实现已集成到 prl_device.c
# 头文件定义了各设备的消息类型和数据结构
###############################################

###### Add required/dependent components ######
# PRL 依赖 OSAL（时间、内存管理）
list(APPEND ADD_REQUIREMENTS osal)
###############################################

#### Add compile option for this component ####
# 协议层编译选项
###############################################

# 注册组件
if(CONFIG_PRL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
