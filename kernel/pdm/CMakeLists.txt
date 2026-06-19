################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "${SDK_PATH}/kernel/include")
list(APPEND ADD_INCLUDE "${SDK_PATH}/kernel/include/pdm")

# 私有头文件目录（仅内部使用）
list(APPEND ADD_PRIVATE_INCLUDE "include")
list(APPEND ADD_PRIVATE_INCLUDE "src")
list(APPEND ADD_PRIVATE_INCLUDE "src/protocol")
###############################################

############## Add source files ###############
list(APPEND ADD_SRCS
    "src/pdm.c"
    "src/base/pdm_bus.c"
    "src/base/pdm_chrdev.c"
    "src/base/pdm_driver.c"
    "src/base/pdm_driver_start.c"
    "src/base/pdm_proc.c"
    "src/base/pdm_status.c"
)

# 根据配置添加子模块源文件
if(CONFIG_PDM_PROTOCOL)
    list(APPEND ADD_SRCS
        "src/protocol/pdm_protocol.c"
        "src/protocol/pdm_protocol_common.c"
    )
endif()

if(CONFIG_PDM_MCU_SUPPORT AND TARGET hal)
    list(APPEND ADD_SRCS
        "src/mcu/pdm_mcu.c"
        "src/mcu/pdm_mcu_chrdev.c"
        "src/mcu/pdm_mcu_can.c"
        "src/mcu/pdm_mcu_serial.c"
    )
endif()

if(CONFIG_PDM_LED_SUPPORT AND TARGET hal)
    list(APPEND ADD_SRCS
        "src/led/pdm_led.c"
        "src/led/pdm_led_chrdev.c"
    )
endif()

list(APPEND ADD_SRCS "src/base/pdm_driver_end.c")
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

# HAL is kernel-module-only. CMake host builds link the HAL-dependent MCU
# sources only when a test double/legacy target is explicitly present.
if(CONFIG_PDM_MCU_SUPPORT AND TARGET hal)
    list(APPEND ADD_REQUIREMENTS hal)
endif()

if(CONFIG_PDM_LED_SUPPORT AND TARGET hal)
    list(APPEND ADD_REQUIREMENTS hal)
endif()

# PCONFIG 是必须的（PDM 使用 PCONFIG 的配置类型）
if(CONFIG_PCONFIG)
    list(APPEND ADD_REQUIREMENTS pconfig)
else()
    message(FATAL_ERROR "PDM requires PCONFIG but CONFIG_PCONFIG is not enabled")
endif()

###############################################

# 注册组件（使用公共函数）
list(APPEND ADD_INCLUDE "${SDK_PATH}/kernel/include")
list(APPEND ADD_INCLUDE "${SDK_PATH}/kernel/include/pdm")
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

register_component()
