# =============================================================================
# EMS CMake 构建系统
# =============================================================================
# 版本: 3.0
# 创建日期: 2026-05-27
# 说明: 使用 CMake 构建系统，保留 Kconfig 配置功能
# =============================================================================

cmake_minimum_required(VERSION 3.16)

project(EMS
    VERSION 1.0.0
    DESCRIPTION "Embedded Management System"
    LANGUAGES C
)

# =============================================================================
# 构建选项
# =============================================================================

# 设置 C 标准（使用 C99，支持 // 注释和更多特性）
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Compiler version: ${CMAKE_C_COMPILER_VERSION}")

# =============================================================================
# Kconfig 集成
# =============================================================================

# 包含 Kconfig 模块
include(cmake/Kconfig.cmake)

# 加载 Kconfig 配置
load_kconfig("${CMAKE_SOURCE_DIR}/.config")

# 配置工具链（必须在 Kconfig 加载后）
include(cmake/Toolchain.cmake)

# 生成 autoconf.h（兼容现有代码）
set(AUTOCONF_H "${CMAKE_BINARY_DIR}/include/generated/autoconf.h")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include/generated")
export_kconfig_to_c("${AUTOCONF_H}")

# 添加到包含路径
include_directories("${CMAKE_BINARY_DIR}/include")

# =============================================================================
# 全局编译选项（根据 Kconfig 配置）
# =============================================================================

include(cmake/CompilerOptions.cmake)

# 全局宏定义
add_compile_definitions(__EMS__)

# POSIX 特性测试宏（启用 POSIX 扩展）
add_compile_definitions(
    _POSIX_C_SOURCE=200809L
    _XOPEN_SOURCE=700
    _GNU_SOURCE
)

# =============================================================================
# 输出目录设置
# =============================================================================

# 设置库和可执行文件输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 创建 staging 目录（兼容现有脚本）
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/.staging/lib)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/.staging/bin)

# =============================================================================
# 子目录（根据 Kconfig 配置）
# =============================================================================

message(STATUS "")
message(STATUS "=== Building Core Modules ===")

# Core 模块
if(CONFIG_OSAL)
    message(STATUS "Building OSAL")
    add_subdirectory(core/osal)
else()
    message(STATUS "OSAL disabled")
endif()

if(CONFIG_HAL)
    message(STATUS "Building HAL")
    add_subdirectory(core/hal)
else()
    message(STATUS "HAL disabled")
endif()

if(CONFIG_PCL)
    message(STATUS "Building PCL")
    add_subdirectory(core/pcl)
else()
    message(STATUS "PCL disabled")
endif()

if(CONFIG_PDL)
    message(STATUS "Building PDL")
    add_subdirectory(core/pdl)
else()
    message(STATUS "PDL disabled")
endif()

if(CONFIG_ACL)
    message(STATUS "Building ACL")
    add_subdirectory(core/acl)
else()
    message(STATUS "ACL disabled")
endif()

message(STATUS "")
message(STATUS "=== Building Products ===")

# Products 模块
add_subdirectory(products/ccm)

message(STATUS "")
message(STATUS "=== Building Tests ===")

# Tests 模块
if(CONFIG_BUILD_TESTING)
    message(STATUS "Building tests")
    add_subdirectory(tests)
else()
    message(STATUS "Tests disabled")
endif()

# =============================================================================
# 安装规则
# =============================================================================

# 安装库文件
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/
    DESTINATION lib
    FILES_MATCHING
    PATTERN "*.so*"
    PATTERN "*.a"
)

# 安装可执行文件
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
    DESTINATION bin
    FILES_MATCHING
    PATTERN "*"
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                GROUP_EXECUTE GROUP_READ
                WORLD_EXECUTE WORLD_READ
)

# 安装头文件
install(DIRECTORY ${CMAKE_BINARY_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# =============================================================================
# 构建信息
# =============================================================================

message(STATUS "")
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Library output: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "Binary output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "")
