# OSAL (操作系统抽象层) 模块
# 提供接口库和实现库

# ============================================================================
# 1. 接口库 (osal_public_api) - 仅头文件，供上层模块使用
# ============================================================================

# 目录变量
set(OSAL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/posix)
set(OSAL_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(osal_public_api INTERFACE)

target_include_directories(osal_public_api INTERFACE
    $<BUILD_INTERFACE:${OSAL_INC_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ============================================================================
# 2. 实现库 (osal) - 包含平台相关实现
# ============================================================================

# OSAL源文件
set(OSAL_SOURCES
    # 基础设施
    ${OSAL_SRC_DIR}/lib/osal_errno.c

    # IPC - 进程间通信
    ${OSAL_SRC_DIR}/ipc/osal_mutex.c
    ${OSAL_SRC_DIR}/ipc/osal_atomic.c
    ${OSAL_SRC_DIR}/ipc/osal_semaphore.c
    ${OSAL_SRC_DIR}/ipc/osal_cond.c

    # SYS - 系统调用封装
    ${OSAL_SRC_DIR}/sys/osal_clock.c
    ${OSAL_SRC_DIR}/sys/osal_signal.c
    ${OSAL_SRC_DIR}/sys/osal_file.c
    ${OSAL_SRC_DIR}/sys/osal_select.c
    ${OSAL_SRC_DIR}/sys/osal_env.c
    ${OSAL_SRC_DIR}/sys/osal_time.c
    ${OSAL_SRC_DIR}/sys/osal_process.c
    ${OSAL_SRC_DIR}/sys/osal_thread.c

    # NET - 网络相关
    ${OSAL_SRC_DIR}/net/osal_socket.c
    ${OSAL_SRC_DIR}/net/osal_termios.c

    # LIB - 标准库封装
    ${OSAL_SRC_DIR}/lib/osal_heap.c
    ${OSAL_SRC_DIR}/lib/osal_string.c
    ${OSAL_SRC_DIR}/lib/osal_stdio.c
    ${OSAL_SRC_DIR}/lib/osal_errno.c

    # UTIL - 工具类
    ${OSAL_SRC_DIR}/util/osal_version.c
    ${OSAL_SRC_DIR}/util/osal_log.c
)

# 创建OSAL库（类型由BUILD_SHARED_LIBS控制）
add_library(osal ${OSAL_SOURCES})

# 链接接口库（继承头文件路径）
target_link_libraries(osal PUBLIC osal_public_api)

# 链接系统库
if(APPLE)
    # macOS 不需要 librt (实时扩展已集成到系统库中)
    target_link_libraries(osal PUBLIC
        Threads::Threads
    )
else()
    # Linux 需要 librt (POSIX 实时扩展)
    target_link_libraries(osal PUBLIC
        Threads::Threads
        rt
    )
endif()

# 设置编译选项
target_compile_options(osal PRIVATE
    -Wall
    -Wextra
)

# ============================================================================
# 3. 安装规则（使用 GNUInstallDirs 变量）
# ============================================================================
install(TARGETS osal osal_public_api
    EXPORT osal-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(INSTALL_HEADERS)
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/osal
        FILES_MATCHING PATTERN "*.h"
    )
endif()

install(EXPORT osal-targets
    FILE osal-targets.cmake
    NAMESPACE ems::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osal
)

message(STATUS "OSAL configured")
