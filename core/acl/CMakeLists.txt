# =============================================================================
# ACL (Access Control Layer) CMake 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 收集源文件（根据 Kconfig 条件编译）
# -----------------------------------------------------------------------------
include(config.cmake)

# -----------------------------------------------------------------------------
# 2. 创建共享库
# -----------------------------------------------------------------------------
add_library(acl SHARED ${ACL_SRCS})

# -----------------------------------------------------------------------------
# 3. 头文件路径
# -----------------------------------------------------------------------------
target_include_directories(acl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# -----------------------------------------------------------------------------
# 4. 链接依赖
# -----------------------------------------------------------------------------
target_link_libraries(acl
    PUBLIC
        pcl
        hal
        osal
)

# 如果 PDL 存在，则链接
if(TARGET pdl)
    target_link_libraries(acl PUBLIC pdl)
endif()

# -----------------------------------------------------------------------------
# 5. 创建静态库
# -----------------------------------------------------------------------------
add_library(acl_static STATIC ${ACL_SRCS})

set_target_properties(acl_static PROPERTIES
    OUTPUT_NAME acl
)

target_include_directories(acl_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(acl_static
    PUBLIC
        pcl_static
        hal_static
        osal_static
)

# 如果 PDL 存在，则链接
if(TARGET pdl_static)
    target_link_libraries(acl_static PUBLIC pdl_static)
endif()

# -----------------------------------------------------------------------------
# 6. 安装
# -----------------------------------------------------------------------------
install(TARGETS acl acl_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
