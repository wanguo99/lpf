# =============================================================================
# PDM 模块配置
# =============================================================================
# 功能：根据 Kconfig 配置选择需要编译的源文件
# 用法：include(config.cmake)
#       然后使用 ${PDM_SRCS} 变量
# =============================================================================

message(STATUS "Configuring PDM module...")

set(PDM_SRCS "")

list(APPEND PDM_SRCS
    "src/pdm.c"
    "src/pdm_driver_start.c"
)

if(CONFIG_PDM_PROTOCOL)
    list(APPEND PDM_SRCS
        "src/protocol/pdm_protocol.c"
        "src/protocol/pdm_protocol_common.c"
        "src/protocol/pdm_protocol_device.c"
    )
    message(STATUS "  [PDM] Internal protocol support enabled")
endif()

# MCU 外设支持（根据 Kconfig 配置）
if(CONFIG_PDM_MCU_SUPPORT)
    list(APPEND PDM_SRCS
        "src/pdm_mcu/pdm_mcu.c"
        "src/pdm_mcu/pdm_mcu_chrdev.c"
        "src/pdm_mcu/pdm_mcu_can.c"
        "src/pdm_mcu/pdm_mcu_serial.c"
    )
    message(STATUS "  [PDM] MCU peripheral support enabled")
endif()

list(APPEND PDM_SRCS "src/pdm_driver_end.c")

list(LENGTH PDM_SRCS PDM_FILE_COUNT)
message(STATUS "  [PDM] Total ${PDM_FILE_COUNT} source files selected")
