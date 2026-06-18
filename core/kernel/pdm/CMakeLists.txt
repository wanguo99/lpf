################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "${SDK_PATH}/core/kernel/include/pdm")

# 私有头文件目录（仅内部使用）
list(APPEND ADD_PRIVATE_INCLUDE "src")
###############################################

############## Add source files ###############
# 根据配置添加子模块源文件
if(CONFIG_PDM_MCU_SUPPORT)
    file(GLOB PDM_MCU_SRCS "src/pdm_mcu/*.c")
    list(APPEND ADD_SRCS ${PDM_MCU_SRCS})
endif()
###############################################

###### Add required/dependent components ######
# PDM 依赖 OSAL、HAL 和 PCONFIG
# PDM 从 PCONFIG 获取配置类型和运行时配置
# PCONFIG 定义配置类型，PDM 使用
# 依赖关系：PDM→PCONFIG（单向依赖，无循环）

# OSAL 是必须的（Kconfig: select OSAL）
if(CONFIG_OSAL)
    list(APPEND ADD_REQUIREMENTS osal)
else()
    message(FATAL_ERROR "PDM requires OSAL but CONFIG_OSAL is not enabled")
endif()

# HAL 是 Kconfig 强制依赖的（Kconfig: select HAL）
if(CONFIG_HAL)
    list(APPEND ADD_REQUIREMENTS hal)
else()
    message(FATAL_ERROR "PDM requires HAL but CONFIG_HAL is not enabled")
endif()

# PCONFIG 是必须的（PDM 使用 PCONFIG 的配置类型）
if(CONFIG_PCONFIG)
    list(APPEND ADD_REQUIREMENTS pconfig)
else()
    message(FATAL_ERROR "PDM requires PCONFIG but CONFIG_PCONFIG is not enabled")
endif()

# PRL 是条件依赖的（仅 MCU 外设需要）
if(CONFIG_PDM_MCU_SUPPORT)
    if(CONFIG_PRL)
        list(APPEND ADD_REQUIREMENTS prl)
    else()
        message(FATAL_ERROR "PDM_MCU requires PRL but CONFIG_PRL is not enabled")
    endif()
endif()
###############################################

#### Add compile option for this component ####
if(CONFIG_PDM_MCU_SUPPORT)
    list(APPEND ADD_DEFINITIONS -DPDM_MCU_SUPPORT=1)
endif()
###############################################

# 注册组件（使用公共函数）
list(APPEND ADD_INCLUDE "${SDK_PATH}/core/kernel/include/pdm")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_PDM_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
