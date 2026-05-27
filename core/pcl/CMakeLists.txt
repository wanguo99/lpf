# =============================================================================
# PCL (Protocol Control Layer) CMake 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 收集源文件（根据 Kconfig 条件编译）
# -----------------------------------------------------------------------------
include(config.cmake)

# -----------------------------------------------------------------------------
# 2. 创建共享库
# -----------------------------------------------------------------------------
add_library(pcl SHARED ${PCL_SRCS})

# 设置库版本
set_target_properties(pcl PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME pcl
)

# -----------------------------------------------------------------------------
# 3. 头文件路径
# -----------------------------------------------------------------------------
target_include_directories(pcl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# -----------------------------------------------------------------------------
# 4. 链接依赖
# -----------------------------------------------------------------------------
target_link_libraries(pcl
    PUBLIC
        hal
        osal
)

# -----------------------------------------------------------------------------
# 5. 创建静态库
# -----------------------------------------------------------------------------
add_library(pcl_static STATIC ${PCL_SRCS})

set_target_properties(pcl_static PROPERTIES
    OUTPUT_NAME pcl
)

target_include_directories(pcl_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pcl_static
    PUBLIC
        hal_static
        osal_static
)

# -----------------------------------------------------------------------------
# 6. 安装
# -----------------------------------------------------------------------------
install(TARGETS pcl pcl_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
