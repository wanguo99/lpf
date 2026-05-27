# =============================================================================
# PDL (Protocol Data Layer) CMake 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 收集源文件（根据 Kconfig 条件编译）
# -----------------------------------------------------------------------------
include(config.cmake)

# -----------------------------------------------------------------------------
# 2. 创建共享库
# -----------------------------------------------------------------------------
if(PDL_SRCS)
    add_library(pdl SHARED ${PDL_SRCS})
else()
    message(STATUS "  [PDL] Skipping shared library (no sources)")
endif()

# -----------------------------------------------------------------------------
# 3. 头文件路径
# -----------------------------------------------------------------------------
if(TARGET pdl)
    target_include_directories(pdl
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
endif()

# -----------------------------------------------------------------------------
# 4. 链接依赖
# -----------------------------------------------------------------------------
if(TARGET pdl)
    target_link_libraries(pdl
        PUBLIC
            pcl
            hal
            osal
    )
endif()

# -----------------------------------------------------------------------------
# 5. 创建静态库
# -----------------------------------------------------------------------------
if(PDL_SRCS)
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
else()
    message(STATUS "  [PDL] Skipping static library (no sources)")
endif()

# -----------------------------------------------------------------------------
# 6. 安装
# -----------------------------------------------------------------------------
if(TARGET pdl)
    install(TARGETS pdl pdl_static
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
