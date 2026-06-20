################# Add include #################
# 公共头文件目录（对外暴露）
list(APPEND ADD_INCLUDE "${SDK_PATH}/kernel/include")
list(APPEND ADD_INCLUDE "${SDK_PATH}/kernel/include/pdm")

list(APPEND ADD_PRIVATE_INCLUDE "${SDK_PATH}/kernel/lpf/protocol")
list(APPEND ADD_PRIVATE_INCLUDE "${SDK_PATH}/kernel/lpf/peripheral/mcu")
list(APPEND ADD_PRIVATE_INCLUDE "${SDK_PATH}/kernel/lpf/peripheral/led")
list(APPEND ADD_PRIVATE_INCLUDE "${SDK_PATH}/kernel/lpf/transport/mcu")
###############################################

############## Add source files ###############
list(APPEND ADD_SRCS
    "src/pdm.c"
    "../lpf/peripheral/lpf_peripheral.c"
    "../lpf/peripheral/lpf_peripheral_config.c"
)

# 根据配置添加子模块源文件
if(CONFIG_LPF_PROTOCOL)
    list(APPEND ADD_SRCS
        "../lpf/protocol/lpf_protocol.c"
        "../lpf/protocol/lpf_protocol_common.c"
    )
endif()

if(CONFIG_LPF_MCU_SERVICE AND TARGET hal)
    list(APPEND ADD_SRCS
        "../lpf/peripheral/mcu/lpf_mcu_service.c"
        "../lpf/peripheral/mcu/lpf_mcu_chrdev.c"
        "../lpf/peripheral/mcu/lpf_mcu_proc.c"
        "../lpf/transport/mcu/lpf_mcu_transport.c"
    )
endif()

if(CONFIG_LPF_MCU_TRANSPORT_CAN AND TARGET hal)
    list(APPEND ADD_SRCS
        "../lpf/transport/mcu/lpf_mcu_transport_can.c"
    )
endif()

if(CONFIG_LPF_MCU_TRANSPORT_UART AND TARGET hal)
    list(APPEND ADD_SRCS
        "../lpf/transport/mcu/lpf_mcu_transport_uart.c"
    )
endif()

if(CONFIG_LPF_LED_SERVICE AND TARGET hal)
    list(APPEND ADD_SRCS
        "../lpf/peripheral/led/lpf_led_service.c"
        "../lpf/peripheral/led/lpf_led_chrdev.c"
        "../lpf/peripheral/led/lpf_led_proc.c"
    )
endif()

###############################################

###### Add required/dependent components ######
# PDM 集成 OSAL、LPF Core、HAL、PCONFIG 和 LPF peripheral 层。
# PCONFIG 到 LPF device 的转换由 LPF peripheral 配置入口完成。

# OSAL 是必须的（Kconfig: select OSAL）
if(CONFIG_OSAL)
    list(APPEND ADD_REQUIREMENTS osal)
else()
    message(FATAL_ERROR "PDM requires OSAL but CONFIG_OSAL is not enabled")
endif()

if(CONFIG_LPF_CORE)
    list(APPEND ADD_REQUIREMENTS lpf_core)
else()
    message(FATAL_ERROR "PDM requires LPF Core but CONFIG_LPF_CORE is not enabled")
endif()

# HAL is kernel-module-only. CMake host builds link the HAL-dependent MCU
# sources only when a test double/legacy target is explicitly present.
if(CONFIG_LPF_MCU_SERVICE AND TARGET hal)
    list(APPEND ADD_REQUIREMENTS hal)
endif()

if(CONFIG_LPF_LED_SERVICE AND TARGET hal)
    list(APPEND ADD_REQUIREMENTS hal)
endif()

# PCONFIG 是必须的（LPF peripheral 配置入口使用 PCONFIG 的配置类型）
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
