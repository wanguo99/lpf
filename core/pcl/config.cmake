# =============================================================================
# PCL 模块配置
# =============================================================================
# 功能：根据 Kconfig 配置选择需要编译的源文件
# 用法：include(config.cmake)
#       然后使用 ${PCL_SRCS} 变量
# =============================================================================

message(STATUS "Configuring PCL module...")

# 初始化源文件列表
set(PCL_SRCS "")

# =============================================================================
# 核心源文件（总是需要）
# =============================================================================
list(APPEND PCL_SRCS
    "src/pcl_api.c"
    "src/pcl_register.c"
)

message(STATUS "  [PCL] Core API enabled")

# 验证源文件
if(NOT PCL_SRCS)
    message(FATAL_ERROR "PCL: No source files selected.")
endif()

list(LENGTH PCL_SRCS PCL_FILE_COUNT)
message(STATUS "  [PCL] Total ${PCL_FILE_COUNT} source files selected")
