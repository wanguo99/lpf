# =============================================================================
# PDL (Protocol Data Layer) CMake 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 收集源文件
# -----------------------------------------------------------------------------
file(GLOB PDL_SRCS
    "src/*.c"
)

# -----------------------------------------------------------------------------
# 2. 创建共享库
# -----------------------------------------------------------------------------
add_library(pdl SHARED ${PDL_SRCS})

# 设置库版本
set_target_properties(pdl PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME pdl
)

# -----------------------------------------------------------------------------
# 3. 头文件路径
# -----------------------------------------------------------------------------
target_include_directories(pdl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# -----------------------------------------------------------------------------
# 4. 链接依赖
# -----------------------------------------------------------------------------
target_link_libraries(pdl
    PUBLIC
        pcl
        hal
        osal
)

# -----------------------------------------------------------------------------
# 5. 创建静态库
# -----------------------------------------------------------------------------
add_library(pdl_static STATIC ${PDL_SRCS})

set_target_properties(pdl_static PROPERTIES
    OUTPUT_NAME pdl
)

target_include_directories(pdl_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pdl_static
    PUBLIC
        pcl_static
        hal_static
        osal_static
)

# -----------------------------------------------------------------------------
# 6. 安装
# -----------------------------------------------------------------------------
install(TARGETS pdl pdl_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
