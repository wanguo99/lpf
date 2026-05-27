# =============================================================================
# 工具链配置
# =============================================================================
# 功能：根据 Kconfig 配置设置交叉编译工具链
# 用法：include(cmake/Toolchain.cmake)
# 注意：必须在 project() 之前调用
# =============================================================================

message(STATUS "Configuring toolchain...")

# =============================================================================
# 交叉编译工具链（根据 Kconfig 配置）
# =============================================================================

if(DEFINED CONFIG_CROSS_COMPILE AND NOT CONFIG_CROSS_COMPILE STREQUAL "")
    set(CMAKE_C_COMPILER "${CONFIG_CROSS_COMPILE}gcc")
    set(CMAKE_CXX_COMPILER "${CONFIG_CROSS_COMPILE}g++")
    set(CMAKE_AR "${CONFIG_CROSS_COMPILE}ar")
    set(CMAKE_RANLIB "${CONFIG_CROSS_COMPILE}ranlib")
    set(CMAKE_STRIP "${CONFIG_CROSS_COMPILE}strip")

    message(STATUS "  [Toolchain] Cross-compiling with: ${CONFIG_CROSS_COMPILE}")
    message(STATUS "  [Toolchain] C Compiler: ${CMAKE_C_COMPILER}")
else()
    message(STATUS "  [Toolchain] Native compilation")
endif()
