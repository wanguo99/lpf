# =============================================================================
# ACL 模块配置
# =============================================================================
# 功能：根据 Kconfig 配置选择需要编译的源文件
# 用法：include(config.cmake)
#       然后使用 ${ACL_SRCS} 变量
# =============================================================================

message(STATUS "Configuring ACL module...")

# 初始化源文件列表
set(ACL_SRCS "")

# =============================================================================
# 核心源文件（总是需要）
# =============================================================================
list(APPEND ACL_SRCS
    "src/acl_api.c"
    "src/acl_api_v2.c"
    "src/acl_telemetry_cache.c"
)

message(STATUS "  [ACL] Core API enabled")
message(STATUS "  [ACL] API v2 enabled")
message(STATUS "  [ACL] Telemetry cache enabled")

# 验证源文件
if(NOT ACL_SRCS)
    message(FATAL_ERROR "ACL: No source files selected.")
endif()

list(LENGTH ACL_SRCS ACL_FILE_COUNT)
message(STATUS "  [ACL] Total ${ACL_FILE_COUNT} source files selected")
