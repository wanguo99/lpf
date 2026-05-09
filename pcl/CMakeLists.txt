# PCL (Peripheral Configuration Library) 模块
# 纯数据配置库，类似设备树

# ============================================================================
# 1. 接口库 (pcl_public_api) - 仅头文件，供上层模块使用
# ============================================================================

# 目录变量
set(PCL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PCL_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PCL_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/platform)

add_library(pcl_public_api INTERFACE)

target_include_directories(pcl_public_api INTERFACE
    $<BUILD_INTERFACE:${PCL_INC_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# 2. 实现库 (pcl) - 包含配置数据和API实现
# ============================================================================

# 核心源文件
set(PCL_SOURCES
    ${PCL_SRC_DIR}/pcl_api.c
    ${PCL_SRC_DIR}/pcl_register.c
)

# 平台配置源文件（递归查找所有平台配置）
file(GLOB_RECURSE PLATFORM_CONFIGS
    ${PCL_PLATFORM_DIR}/ti/am6254/*/*.c
    ${PCL_PLATFORM_DIR}/vendor_demo/*/*/*/*.c
    # 其他平台配置可以在这里添加
    # ${PCL_PLATFORM_DIR}/xilinx/zynq/*/*.c
)

# 创建PCL库（类型由BUILD_SHARED_LIBS控制）
add_library(pcl
    ${PCL_SOURCES}
    ${PLATFORM_CONFIGS}
)

# 链接接口库（继承头文件路径）
target_link_libraries(pcl PUBLIC pcl_public_api)

# 内部需要访问自己的头文件（api/internal/peripheral子目录）
target_include_directories(pcl PRIVATE
    ${PCL_INC_DIR}/api
    ${PCL_INC_DIR}/internal
    ${PCL_INC_DIR}/peripheral
)

# 依赖OSAL接口库（仅头文件依赖）
target_link_libraries(pcl PUBLIC ems::osal_public_api)

# 链接OSAL实现库（运行时依赖）
target_link_libraries(pcl PRIVATE ems::osal)

# 设置编译选项
set(CMAKE_C_STANDARD 99)
target_compile_options(pcl PRIVATE
    -Wall
    -Wextra
    -Werror
)

# ============================================================================
# 3. 安装规则（使用 GNUInstallDirs 变量）
# ============================================================================
install(TARGETS pcl pcl_public_api
    EXPORT pcl-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(INSTALL_HEADERS)
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pcl
        FILES_MATCHING PATTERN "*.h"
    )
endif()

install(EXPORT pcl-targets
    FILE pcl-targets.cmake
    NAMESPACE ems::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pcl
)

# ============================================================================
# 4. 示例程序（可选）
# ============================================================================
option(BUILD_PCL_EXAMPLES "Build pcl examples" OFF)

if(BUILD_PCL_EXAMPLES)
    add_executable(pcl_example
        examples/pcl_example.c
    )

    target_link_libraries(pcl_example
        ems::pcl
        ems::osal
        pthread
    )

    install(TARGETS pcl_example
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

list(LENGTH PLATFORM_CONFIGS config_count)
message(STATUS "PCL configured (${config_count} platform configs)")
