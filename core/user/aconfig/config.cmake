# =============================================================================
# ACONFIG 模块配置
# =============================================================================
# 功能：根据 Kconfig 配置选择需要编译的源文件
# 用法：include(config.cmake)
#       然后使用 ${ACONFIG_SRCS} 变量
# =============================================================================

message(STATUS "Configuring ACONFIG module...")

# 初始化源文件列表
set(ACONFIG_SRCS "")

# =============================================================================
# 核心源文件（总是需要）
# =============================================================================
list(APPEND ACONFIG_SRCS
    "src/aconfig_api.c"
)

message(STATUS "  [ACONFIG] Core API enabled")

# 验证源文件
if(NOT ACONFIG_SRCS)
    message(FATAL_ERROR "ACONFIG: No source files selected.")
endif()

list(LENGTH ACONFIG_SRCS ACONFIG_FILE_COUNT)
message(STATUS "  [ACONFIG] Total ${ACONFIG_FILE_COUNT} source files selected")
