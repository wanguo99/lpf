# =============================================================================
# HAL (Hardware Abstraction Layer) CMake 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 确定平台实现目录
# -----------------------------------------------------------------------------
if(CONFIG_HAL_PLATFORM_GENERIC_LINUX)
    set(HAL_PLATFORM_DIR "generic-linux")
elseif(CONFIG_HAL_PLATFORM_TI_AM62X)
    set(HAL_PLATFORM_DIR "ti-am62x")
else()
    set(HAL_PLATFORM_DIR "generic-linux")
endif()

message(STATUS "  HAL platform implementation: ${HAL_PLATFORM_DIR}")

# -----------------------------------------------------------------------------
# 2. 收集源文件
# -----------------------------------------------------------------------------
file(GLOB HAL_SRCS
    "src/${HAL_PLATFORM_DIR}/*.c"
)

# -----------------------------------------------------------------------------
# 3. 创建共享库
# -----------------------------------------------------------------------------
add_library(hal SHARED ${HAL_SRCS})

# 设置库版本
set_target_properties(hal PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME hal
)

# -----------------------------------------------------------------------------
# 4. 头文件路径
# -----------------------------------------------------------------------------
target_include_directories(hal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# -----------------------------------------------------------------------------
# 5. 链接依赖
# -----------------------------------------------------------------------------
target_link_libraries(hal
    PUBLIC
        osal
)

# -----------------------------------------------------------------------------
# 6. 创建静态库
# -----------------------------------------------------------------------------
add_library(hal_static STATIC ${HAL_SRCS})

set_target_properties(hal_static PROPERTIES
    OUTPUT_NAME hal
)

target_include_directories(hal_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(hal_static
    PUBLIC
        osal_static
)

# -----------------------------------------------------------------------------
# 7. 安装
# -----------------------------------------------------------------------------
install(TARGETS hal hal_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
