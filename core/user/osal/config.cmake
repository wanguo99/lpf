# =============================================================================
# OSAL 模块配置
# =============================================================================
# 功能：根据 Kconfig 配置选择需要编译的源文件
# 用法：include(config.cmake)
#       然后使用 ${OSAL_SRCS} 变量
# =============================================================================

message(STATUS "Configuring OSAL module...")

# 初始化源文件列表
set(OSAL_SRCS "")

# =============================================================================
# 核心库文件（总是需要）
# =============================================================================
list(APPEND OSAL_SRCS
    "src/${OSAL_OS_DIR}/lib/osal_errno.c"
    "src/${OSAL_OS_DIR}/lib/osal_heap.c"
    "src/${OSAL_OS_DIR}/lib/osal_stdio.c"
    "src/${OSAL_OS_DIR}/lib/osal_string.c"
    "src/${OSAL_OS_DIR}/util/osal_log.c"
    "src/${OSAL_OS_DIR}/util/osal_version.c"
)

# =============================================================================
# IPC 模块（根据 Kconfig 配置）
# =============================================================================
if(CONFIG_OSAL_IPC)
    if(CONFIG_OSAL_IPC_ATOMIC)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/ipc/osal_atomic.c")
        message(STATUS "  [OSAL] IPC: Atomic operations enabled")
    endif()

    if(CONFIG_OSAL_IPC_MUTEX)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/ipc/osal_mutex.c")
        message(STATUS "  [OSAL] IPC: Mutex enabled")
    endif()

    if(CONFIG_OSAL_IPC_SEMAPHORE)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/ipc/osal_semaphore.c")
        message(STATUS "  [OSAL] IPC: Semaphore enabled")
    endif()

    if(CONFIG_OSAL_IPC_COND)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/ipc/osal_cond.c")
        message(STATUS "  [OSAL] IPC: Condition variable enabled")
    endif()

    if(CONFIG_OSAL_IPC_SHM)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/ipc/osal_shm.c")
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/ipc/osal_shm_highlevel.c")
        message(STATUS "  [OSAL] IPC: Shared memory enabled")
    endif()

    if(CONFIG_OSAL_IPC_SHM_CACHE)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/ipc/osal_shm_cache.c")
        message(STATUS "  [OSAL] IPC: Shared memory cache enabled")
    endif()
endif()

# =============================================================================
# 文件系统模块（根据 Kconfig 配置）
# =============================================================================
if(CONFIG_OSAL_FILE)
    if(CONFIG_OSAL_FILE_CLOCK)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_clock.c")
        message(STATUS "  [OSAL] File: Clock enabled")
    endif()

    if(CONFIG_OSAL_FILE_ENV)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_env.c")
        message(STATUS "  [OSAL] File: Environment enabled")
    endif()

    if(CONFIG_OSAL_FILE_FILE)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_file.c")
        message(STATUS "  [OSAL] File: File operations enabled")
    endif()

    if(CONFIG_OSAL_FILE_PROCESS)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_process.c")
        message(STATUS "  [OSAL] File: Process enabled")
    endif()

    if(CONFIG_OSAL_FILE_SCHED)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_sched.c")
        message(STATUS "  [OSAL] File: Scheduler enabled")
    endif()

    if(CONFIG_OSAL_FILE_SELECT)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_select.c")
        message(STATUS "  [OSAL] File: Select enabled")
    endif()
endif()

# =============================================================================
# 线程模块（根据 Kconfig 配置）
# =============================================================================
if(CONFIG_OSAL_THREAD)
    if(CONFIG_OSAL_THREAD_THREAD)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_thread.c")
        message(STATUS "  [OSAL] Thread: Thread enabled")
    endif()

    if(CONFIG_OSAL_THREAD_TIME)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_time.c")
        message(STATUS "  [OSAL] Thread: Time enabled")
    endif()
endif()

# =============================================================================
# 网络模块（根据 Kconfig 配置）
# =============================================================================
if(CONFIG_OSAL_NETWORK)
    if(CONFIG_OSAL_NETWORK_SOCKET)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/net/osal_socket.c")
        message(STATUS "  [OSAL] Network: Socket enabled")
    endif()

    if(CONFIG_OSAL_NETWORK_TERMIOS)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/net/osal_termios.c")
        message(STATUS "  [OSAL] Network: Termios enabled")
    endif()
endif()

# =============================================================================
# 信号模块（根据 Kconfig 配置）
# =============================================================================
if(CONFIG_OSAL_SIGNAL)
    if(CONFIG_OSAL_SIGNAL_SIGNAL)
        list(APPEND OSAL_SRCS "src/${OSAL_OS_DIR}/sys/osal_signal.c")
        message(STATUS "  [OSAL] Signal: Signal handling enabled")
    endif()
endif()

# 验证至少有核心文件
if(NOT OSAL_SRCS)
    message(FATAL_ERROR "OSAL: No source files selected.")
endif()

list(LENGTH OSAL_SRCS OSAL_FILE_COUNT)
message(STATUS "  [OSAL] Total ${OSAL_FILE_COUNT} source files selected")
