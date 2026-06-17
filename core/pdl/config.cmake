# =============================================================================
# PDL 模块配置
# =============================================================================
# 功能：根据 Kconfig 配置选择需要编译的源文件
# 用法：include(config.cmake)
#       然后使用 ${PDL_SRCS} 变量
# =============================================================================

message(STATUS "Configuring PDL module...")

# 初始化源文件列表
set(PDL_SRCS "")

# =============================================================================
# Satellite 外设支持（根据 Kconfig 配置）
# =============================================================================
if(CONFIG_PDL_SATELLITE_SUPPORT)
    list(APPEND PDL_SRCS
        "src/pdl_satellite/pdl_satellite.c"
        "src/pdl_satellite/pdl_satellite_can.c"
    )
    message(STATUS "  [PDL] Satellite peripheral support enabled")
endif()

# =============================================================================
# BMC 外设支持（根据 Kconfig 配置）
# =============================================================================
if(CONFIG_PDL_BMC_SUPPORT)
    list(APPEND PDL_SRCS
        "src/pdl_bmc/pdl_bmc.c"
        "src/pdl_bmc/pdl_bmc_ipmi.c"
        "src/pdl_bmc/pdl_bmc_redfish.c"
        "src/pdl_bmc/pdl_bmc_transport.c"
    )
    message(STATUS "  [PDL] BMC peripheral support enabled")
endif()

# =============================================================================
# MCU 外设支持（根据 Kconfig 配置）
# =============================================================================
if(CONFIG_PDL_MCU_SUPPORT)
    list(APPEND PDL_SRCS
        "src/pdl_mcu/pdl_mcu.c"
        "src/pdl_mcu/pdl_mcu_can.c"
        "src/pdl_mcu/pdl_mcu_protocol.c"
        "src/pdl_mcu/pdl_mcu_serial.c"
    )
    message(STATUS "  [PDL] MCU peripheral support enabled")
endif()

# 验证至少有一个外设被启用
if(NOT PDL_SRCS)
    message(WARNING "PDL: No peripheral support enabled. Skipping PDL build.")
    message(WARNING "  Enable at least one of: PDL_SATELLITE_SUPPORT, PDL_BMC_SUPPORT, PDL_MCU_SUPPORT")
else()
    list(LENGTH PDL_SRCS PDL_FILE_COUNT)
    message(STATUS "  [PDL] Total ${PDL_FILE_COUNT} source files selected")
endif()
