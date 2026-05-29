################# Add include #################
list(APPEND ADD_INCLUDE "include")
###############################################

############## Add source files ###############
# PRL 核心源文件
list(APPEND ADD_SRCS
    "src/prl_common.c"
)

# GSC-PMC 协议
if(CONFIG_PRL_GSC_PMC)
    list(APPEND ADD_SRCS "src/prl_gsc_pmc.c")
endif()

# PMC-CCM 协议
if(CONFIG_PRL_PMC_CCM)
    list(APPEND ADD_SRCS "src/prl_pmc_ccm.c")
endif()

# CCM-Satellite 协议
if(CONFIG_PRL_CCM_SATELLITE)
    list(APPEND ADD_SRCS "src/prl_ccm_satellite.c")
endif()

# CCM-Power 协议
if(CONFIG_PRL_CCM_POWER)
    list(APPEND ADD_SRCS "src/prl_ccm_power.c")
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
