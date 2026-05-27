# =============================================================================
# OSAL (Operating System Abstraction Layer) CMake 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 确定 OS 实现目录
# -----------------------------------------------------------------------------
if(CONFIG_OSAL_OS_POSIX)
    set(OSAL_OS_DIR "posix")
elseif(CONFIG_OSAL_OS_WIN32)
    set(OSAL_OS_DIR "win32")
elseif(CONFIG_OSAL_OS_RTOS)
    set(OSAL_OS_DIR "rtos")
elseif(CONFIG_OSAL_OS_BARE)
    set(OSAL_OS_DIR "bare")
else()
    set(OSAL_OS_DIR "posix")
endif()

message(STATUS "  OSAL OS implementation: ${OSAL_OS_DIR}")

# -----------------------------------------------------------------------------
# 2. 收集源文件（根据 Kconfig 条件编译）
# -----------------------------------------------------------------------------
include(config.cmake)

# -----------------------------------------------------------------------------
# 3. 创建共享库
# -----------------------------------------------------------------------------
add_library(osal SHARED ${OSAL_SRCS})

# -----------------------------------------------------------------------------
# 4. 头文件路径
# -----------------------------------------------------------------------------
target_include_directories(osal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${OSAL_OS_DIR}
)

# -----------------------------------------------------------------------------
# 5. 编译定义
# -----------------------------------------------------------------------------
if(CONFIG_OSAL_OS_POSIX)
    target_compile_definitions(osal PRIVATE OSAL_OS_POSIX)
endif()

if(CONFIG_OSAL_OS_WIN32)
    target_compile_definitions(osal PRIVATE OSAL_OS_WIN32)
endif()

if(CONFIG_OSAL_OS_RTOS)
    target_compile_definitions(osal PRIVATE OSAL_OS_RTOS)
endif()

if(CONFIG_OSAL_OS_BARE)
    target_compile_definitions(osal PRIVATE OSAL_OS_BARE)
endif()

if(CONFIG_OSAL_ARCH_32BIT)
    target_compile_definitions(osal PRIVATE OSAL_ARCH_32BIT)
endif()

if(CONFIG_OSAL_ARCH_64BIT)
    target_compile_definitions(osal PRIVATE OSAL_ARCH_64BIT)
endif()

# -----------------------------------------------------------------------------
# 6. 链接库
# -----------------------------------------------------------------------------
if(CONFIG_OSAL_OS_POSIX)
    target_link_libraries(osal
        PUBLIC
            pthread
            rt
    )
endif()

if(CONFIG_OSAL_OS_WIN32)
    target_link_libraries(osal
        PUBLIC
            ws2_32
    )
endif()

# -----------------------------------------------------------------------------
# 7. 创建静态库（可选）
# -----------------------------------------------------------------------------
add_library(osal_static STATIC ${OSAL_SRCS})

set_target_properties(osal_static PROPERTIES
    OUTPUT_NAME osal
)

target_include_directories(osal_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${OSAL_OS_DIR}
)

# 复制编译定义
target_compile_definitions(osal_static PRIVATE $<TARGET_PROPERTY:osal,COMPILE_DEFINITIONS>)

# 复制链接库
target_link_libraries(osal_static PUBLIC $<TARGET_PROPERTY:osal,LINK_LIBRARIES>)

# -----------------------------------------------------------------------------
# 8. 安装
# -----------------------------------------------------------------------------
install(TARGETS osal osal_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# -----------------------------------------------------------------------------
# 9. 导出目标（供其他模块使用）
# -----------------------------------------------------------------------------
# 其他模块可以通过 target_link_libraries(xxx PRIVATE osal) 使用
