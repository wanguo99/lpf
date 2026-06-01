################# Add include #################
list(APPEND ADD_INCLUDE "include")
###############################################

############## Add source files ###############
# PRL 核心源文件
list(APPEND ADD_SRCS
    "src/prl_common.c"
    "src/prl_device.c"
    "src/prl_api.c"
)

# MCU 协议（按设备类型组织）
if(CONFIG_PRL_MCU)
    list(APPEND ADD_SRCS "src/prl_mcu.c")
endif()

# CCM 协议（按设备类型组织）
if(CONFIG_PRL_CCM)
    list(APPEND ADD_SRCS "src/prl_ccm.c")
endif()

# PMC 协议（按设备类型组织）
if(CONFIG_PRL_PMC)
    list(APPEND ADD_SRCS "src/prl_pmc.c")
endif()

# GSC 协议（按设备类型组织）
if(CONFIG_PRL_GSC)
    list(APPEND ADD_SRCS "src/prl_gsc.c")
endif()

# POWER 协议（按设备类型组织）
if(CONFIG_PRL_POWER)
    list(APPEND ADD_SRCS "src/prl_power.c")
endif()
###############################################

###### Add required/dependent components ######
# PRL 依赖 OSAL（时间、内存管理）
list(APPEND ADD_REQUIREMENTS osal)
###############################################

#### Add compile option for this component ####
# 协议层编译选项
###############################################

# 注册组件
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_PRL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
