# =============================================================================
# PCONFIG 模块配置
# =============================================================================
# 功能：根据 Kconfig 配置选择需要编译的源文件
# 用法：include(config.cmake)
#       然后使用 ${PCONFIG_SRCS} 变量
# =============================================================================

message(STATUS "Configuring PCONFIG module...")

# 初始化源文件列表
set(PCONFIG_SRCS "")

# =============================================================================
# 核心源文件（总是需要）
# =============================================================================
list(APPEND PCONFIG_SRCS
    "src/pconfig_api.c"
)

message(STATUS "  [PCONFIG] Core API enabled")

# 验证源文件
if(NOT PCONFIG_SRCS)
    message(FATAL_ERROR "PCONFIG: No source files selected.")
endif()

list(LENGTH PCONFIG_SRCS PCONFIG_FILE_COUNT)
message(STATUS "  [PCONFIG] Total ${PCONFIG_FILE_COUNT} source files selected")
