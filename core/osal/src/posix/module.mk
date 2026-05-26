# =============================================================================
# OSAL POSIX 平台源文件配置
# =============================================================================
# 本文件由 core/osal/module.mk 自动包含
# 只需维护本平台的源文件列表

# 基础库文件
osal_SRCS += \
	core/osal/src/posix/lib/osal_errno.c \
	core/osal/src/posix/lib/osal_heap.c \
	core/osal/src/posix/lib/osal_stdio.c \
	core/osal/src/posix/lib/osal_string.c \
	core/osal/src/posix/util/osal_log.c \
	core/osal/src/posix/util/osal_version.c

# IPC 支持
ifeq ($(CONFIG_OSAL_IPC),y)
osal_SRCS += \
	core/osal/src/posix/ipc/osal_atomic.c \
	core/osal/src/posix/ipc/osal_cond.c \
	core/osal/src/posix/ipc/osal_mutex.c \
	core/osal/src/posix/ipc/osal_semaphore.c \
	core/osal/src/posix/ipc/osal_shm.c \
	core/osal/src/posix/ipc/osal_shm_cache.c
endif

# 文件和系统支持
ifeq ($(CONFIG_OSAL_FILE),y)
osal_SRCS += \
	core/osal/src/posix/sys/osal_clock.c \
	core/osal/src/posix/sys/osal_env.c \
	core/osal/src/posix/sys/osal_file.c \
	core/osal/src/posix/sys/osal_process.c \
	core/osal/src/posix/sys/osal_sched.c \
	core/osal/src/posix/sys/osal_select.c \
	core/osal/src/posix/sys/osal_thread.c \
	core/osal/src/posix/sys/osal_time.c \
	core/osal/src/posix/sys/osal_signal.c
endif

# 网络支持
ifeq ($(CONFIG_OSAL_NETWORK),y)
osal_SRCS += \
	core/osal/src/posix/net/osal_socket.c \
	core/osal/src/posix/net/osal_termios.c
endif
