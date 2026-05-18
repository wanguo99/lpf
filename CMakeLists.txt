cmake_minimum_required(VERSION 3.10)

project(EMS VERSION 1.0.0 LANGUAGES C)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================================
# 项目选择
# ============================================================================
set(PROJECT_NAME "pmc_h200" CACHE STRING "Project to build")
set_property(CACHE PROJECT_NAME PROPERTY STRINGS
    "pmc_h200"
    "samples"
)

message(STATUS "Selected project: ${PROJECT_NAME}")

# ============================================================================
# 构建选项
# ============================================================================
option(BUILD_TESTING "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" ON)
option(INSTALL_HEADERS "Install header files" ON)

# ============================================================================
# 安装路径配置（Buildroot 兼容）
# ============================================================================
# 使用 GNUInstallDirs 提供标准安装路径变量
include(GNUInstallDirs)

# 默认安装路径：/usr/local（可通过 -DCMAKE_INSTALL_PREFIX 覆盖）
# Buildroot 通常设置为 /usr
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix" FORCE)
endif()

# Buildroot 兼容说明：
# - CMAKE_INSTALL_PREFIX: 安装前缀（Buildroot 通常设置为 /usr）
# - CMAKE_INSTALL_LIBDIR: 库文件目录（相对路径，如 lib 或 lib64）
# - CMAKE_INSTALL_BINDIR: 可执行文件目录（相对路径，如 bin）
# - CMAKE_INSTALL_INCLUDEDIR: 头文件目录（相对路径，如 include）
# - DESTDIR: 安装时的目标根目录（make install DESTDIR=/staging）

# ============================================================================
# 编译选项
# ============================================================================
# 基础编译标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L")

# macOS平台特定定义
if(APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DARWIN_C_SOURCE")
endif()

# 设置默认构建类型为 Release（Buildroot 通常会覆盖此设置）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug/Release)" FORCE)
endif()

# 构建类型特定的标志（如果 Buildroot 未设置 CMAKE_C_FLAGS_RELEASE）
if(NOT CMAKE_C_FLAGS_DEBUG)
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
endif()
if(NOT CMAKE_C_FLAGS_RELEASE)
    set(CMAKE_C_FLAGS_RELEASE "-O2")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ============================================================================
# 输出目录配置
# ============================================================================
# 使用更简洁的输出目录结构
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ============================================================================
# 查找依赖库
# ============================================================================
find_package(Threads REQUIRED)

# ============================================================================
# 模块化构建
# ============================================================================
# 依赖层次：OSAL → HAL → XConfig → PDL → Apps
#
# 每个模块提供两个库：
# 1. <module>_public_api - 接口库（仅头文件），供上层使用
# 2. <module> - 实现库（静态库），包含实际代码
#
# 上层模块：
# - PUBLIC依赖下层的接口库（继承头文件路径）
# - PRIVATE依赖下层的实现库（运行时链接）

# 1. OSAL层（操作系统抽象层）
add_subdirectory(osal)

# 为后续模块创建命名空间别名
add_library(ems::osal_public_api ALIAS osal_public_api)
add_library(ems::osal ALIAS osal)

# 1.5 Test Library（测试库 - 仅在启用测试时构建）
# 现在由统一的tests/CMakeLists.txt管理

# 2. HAL层（硬件抽象层）
add_subdirectory(hal)
add_library(ems::hal_public_api ALIAS hal_public_api)
add_library(ems::hal ALIAS hal)

# 3. XConfig层（硬件配置库）
add_subdirectory(pcl)
add_library(ems::pcl_public_api ALIAS pcl_public_api)
add_library(ems::pcl ALIAS pcl)

# 4. PDL层（外设驱动层）
add_subdirectory(pdl)
add_library(ems::pdl_public_api ALIAS pdl_public_api)
add_library(ems::pdl ALIAS pdl)

# 4.5. ACL层（业务配置层）
add_subdirectory(acl)
add_library(ems::acl ALIAS acl)

# 5. 项目层（Projects）
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/projects/${PROJECT_NAME})
if(NOT EXISTS ${PROJECT_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "Project '${PROJECT_NAME}' not found at ${PROJECT_DIR}")
endif()

message(STATUS "Building project: ${PROJECT_NAME}")
add_subdirectory(projects/${PROJECT_NAME})

# ============================================================================
# 测试框架（可选）
# ============================================================================
if(BUILD_TESTING)
    # 统一测试框架（包含所有测试模块和运行器）
    add_subdirectory(tests)
endif()

# ============================================================================
# 打印配置信息
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "EMS v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
if(CMAKE_TOOLCHAIN_FILE)
message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Binaries: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
message(STATUS "  Libraries: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
message(STATUS "  Headers: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "")
message(STATUS "Build options:")
message(STATUS "  PROJECT_NAME: ${PROJECT_NAME}")
message(STATUS "  BUILD_TESTING: ${BUILD_TESTING}")
message(STATUS "  BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
message(STATUS "  INSTALL_HEADERS: ${INSTALL_HEADERS}")
message(STATUS "")
message(STATUS "Output: ${CMAKE_BINARY_DIR}")
message(STATUS "========================================")
message(STATUS "")
