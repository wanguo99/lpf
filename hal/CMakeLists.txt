# HAL (硬件抽象层) 模块
# 提供接口库和平台相关实现

# ============================================================================
# 1. 接口库 (hal_public_api) - 仅头文件，供上层模块使用
# ============================================================================

# 目录变量
set(HAL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HAL_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(hal_public_api INTERFACE)

target_include_directories(hal_public_api INTERFACE
    $<BUILD_INTERFACE:${HAL_INC_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# 2. 实现库 (hal) - 包含平台相关驱动
# ============================================================================

# HAL源文件（根据平台选择）
if(APPLE)
    # macOS平台：使用打桩实现（仅用于编译）
    set(HAL_SOURCES
        ${HAL_SRC_DIR}/macos/hal_can.c
        ${HAL_SRC_DIR}/macos/hal_serial.c
        ${HAL_SRC_DIR}/macos/hal_i2c.c
        ${HAL_SRC_DIR}/macos/hal_spi.c
        ${HAL_SRC_DIR}/macos/hal_watchdog.c
    )
    message(STATUS "HAL: Using macOS stub implementation (compile-only)")
else()
    # Linux/其他平台：使用真实硬件实现
    set(HAL_SOURCES
        ${HAL_SRC_DIR}/linux/hal_can.c
        ${HAL_SRC_DIR}/linux/hal_serial.c
        ${HAL_SRC_DIR}/linux/hal_i2c.c
        ${HAL_SRC_DIR}/linux/hal_spi.c
        ${HAL_SRC_DIR}/linux/hal_watchdog.c
    )
    message(STATUS "HAL: Using Linux hardware implementation")
endif()

# 创建HAL库（类型由BUILD_SHARED_LIBS控制）
add_library(hal ${HAL_SOURCES})

# 链接接口库（继承头文件路径）
target_link_libraries(hal PUBLIC hal_public_api)

# 依赖OSAL接口库（仅头文件依赖）
target_link_libraries(hal PUBLIC ems::osal_public_api)

# 链接OSAL实现库（运行时依赖）
target_link_libraries(hal PRIVATE ems::osal)

# 设置编译选项
target_compile_options(hal PRIVATE
    -Wall
    -Wextra
)

# ============================================================================
# 3. 安装规则
# ============================================================================
install(TARGETS hal hal_public_api
    EXPORT hal-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/hal
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT hal-targets
    FILE hal-targets.cmake
    NAMESPACE ems::
    DESTINATION lib/cmake/hal
)

message(STATUS "HAL configured (${PLATFORM})")
