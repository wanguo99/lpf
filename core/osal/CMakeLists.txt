################# Add include #################
# 使用标准的 core 组件 include 路径
###############################################

############## Add source files ###############
# 根据 Kconfig 配置精确添加源文件

# ============================================================================
# Common source files (always included)
# ============================================================================
list(APPEND ADD_SRCS "src/common/osal_version.c")

# ============================================================================
# Platform-specific source files
# ============================================================================

if(CONFIG_OSAL_OS_POSIX)
    # POSIX platform (Linux/macOS)

    # IPC module
    if(CONFIG_OSAL_IPC_ATOMIC)
        list(APPEND ADD_SRCS "src/posix/ipc/osal_atomic.c")
    endif()

    if(CONFIG_OSAL_IPC_MUTEX)
        list(APPEND ADD_SRCS "src/posix/ipc/osal_mutex.c")
    endif()

    if(CONFIG_OSAL_IPC_RWLOCK)
        list(APPEND ADD_SRCS "src/posix/ipc/osal_rwlock.c")
    endif()

    if(CONFIG_OSAL_IPC_SEMAPHORE)
        list(APPEND ADD_SRCS "src/posix/ipc/osal_semaphore.c")
    endif()

    if(CONFIG_OSAL_IPC_COND)
        list(APPEND ADD_SRCS "src/posix/ipc/osal_cond.c")
    endif()

    if(CONFIG_OSAL_IPC_SHM)
        list(APPEND ADD_SRCS "src/posix/ipc/osal_shm.c")
    endif()

    if(CONFIG_OSAL_IPC_SHM_CACHE)
        list(APPEND ADD_SRCS "src/posix/ipc/osal_shm_cache.c")
    endif()

    # Library functions
    if(CONFIG_OSAL_LIB_ERRNO)
        list(APPEND ADD_SRCS "src/posix/lib/osal_errno.c")
    endif()

    if(CONFIG_OSAL_LIB_HEAP)
        list(APPEND ADD_SRCS "src/posix/lib/osal_heap.c")
    endif()

    if(CONFIG_OSAL_LIB_STDIO)
        list(APPEND ADD_SRCS "src/posix/lib/osal_stdio.c")
    endif()

    if(CONFIG_OSAL_LIB_STRING)
        list(APPEND ADD_SRCS "src/posix/lib/osal_string.c")
    endif()

    # System operations
    if(CONFIG_OSAL_SYS_CLOCK)
        list(APPEND ADD_SRCS "src/posix/sys/osal_clock.c")
    endif()

    if(CONFIG_OSAL_SYS_ENV)
        list(APPEND ADD_SRCS "src/posix/sys/osal_env.c")
    endif()

    if(CONFIG_OSAL_SYS_FILE)
        list(APPEND ADD_SRCS "src/posix/sys/osal_file.c")
        # Add Linux-specific file lock support
        list(APPEND ADD_SRCS "src/posix/lib/osal_flock.c")
    endif()

    if(CONFIG_OSAL_SYS_PROCESS)
        list(APPEND ADD_SRCS "src/posix/sys/osal_process.c")
    endif()

    if(CONFIG_OSAL_SYS_SCHED)
        list(APPEND ADD_SRCS "src/posix/sys/osal_sched.c")
    endif()

    if(CONFIG_OSAL_SYS_SELECT)
        list(APPEND ADD_SRCS "src/posix/sys/osal_select.c")
        # Add poll() support (required by some HAL drivers)
        list(APPEND ADD_SRCS "src/posix/sys/osal_poll.c")
    endif()

    if(CONFIG_OSAL_SYS_SIGNAL)
        list(APPEND ADD_SRCS "src/posix/sys/osal_signal.c")
    endif()

    if(CONFIG_OSAL_SYS_THREAD)
        list(APPEND ADD_SRCS "src/posix/sys/osal_thread.c")
    endif()

    if(CONFIG_OSAL_SYS_TIME)
        list(APPEND ADD_SRCS "src/posix/sys/osal_time.c")
    endif()

    # Network operations
    if(CONFIG_OSAL_NET_SOCKET)
        list(APPEND ADD_SRCS "src/posix/net/osal_socket.c")
    endif()

    if(CONFIG_OSAL_NET_TERMIOS)
        list(APPEND ADD_SRCS "src/posix/net/osal_termios.c")
    endif()

    # Utility functions
    if(CONFIG_OSAL_UTIL_LOG)
        list(APPEND ADD_SRCS "src/posix/util/osal_log.c")
    endif()

    if(CONFIG_OSAL_UTIL_VERSION)
        list(APPEND ADD_SRCS "src/posix/util/osal_version.c")
    endif()

    # CRC algorithm (platform-independent)
    if(CONFIG_OSAL_UTIL_CRC)
        list(APPEND ADD_SRCS "src/osal_crc.c")
    endif()

elseif(CONFIG_OSAL_OS_WIN32)
    # Windows platform
    # TODO: Add Windows-specific source files when implemented
    message(WARNING "Windows platform not yet implemented")

elseif(CONFIG_OSAL_OS_RTOS)
    # RTOS platform
    # TODO: Add RTOS-specific source files when implemented
    message(WARNING "RTOS platform not yet implemented")

elseif(CONFIG_OSAL_OS_BARE)
    # Bare metal platform
    # TODO: Add bare metal source files when implemented
    message(WARNING "Bare metal platform not yet implemented")

endif()

# Check if any source files were added
list(LENGTH ADD_SRCS SRCS_COUNT)
if(SRCS_COUNT EQUAL 0)
    message(FATAL_ERROR "No OSAL source files selected. Please check your Config.in configuration.")
endif()

message(STATUS "OSAL: Selected ${SRCS_COUNT} source files for compilation")
###############################################

###### Add required/dependent components ######
# OSAL 是基础层，不依赖其他组件
# 但需要系统库
if(CONFIG_OSAL_OS_POSIX)
    list(APPEND ADD_REQUIREMENTS pthread rt m)
endif()
###############################################

#### Add compile option for this component ####
# 根据配置添加编译选项
if(CONFIG_OSAL_DEBUG_LOGGING)
    list(APPEND ADD_DEFINITIONS -DOSAL_DEBUG_LOGGING=1)
endif()

# 添加 POSIX 特性宏
if(CONFIG_OSAL_OS_POSIX)
    list(APPEND ADD_DEFINITIONS
        -D_POSIX_C_SOURCE=200809L
        -D_XOPEN_SOURCE=700
        -D_GNU_SOURCE
    )
endif()
###############################################

# 注册组件（使用公共函数）
# Public API headers (对外公开)
list(APPEND ADD_INCLUDE "include/osal")
list(APPEND ADD_INCLUDE "include/osal/ipc")
list(APPEND ADD_INCLUDE "include/osal/lib")
list(APPEND ADD_INCLUDE "include/osal/net")
list(APPEND ADD_INCLUDE "include/osal/sys")
list(APPEND ADD_INCLUDE "include/osal/util")
# Internal headers (在各自的源文件目录中，通过相对路径引用)
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
list(APPEND ADD_INCLUDE "${PARENT_DIR}")

if(CONFIG_OSAL_BUILD_SHARED)
    register_component(SHARED)
else()
    register_component()
endif()
