# =============================================================================
# OSAL POSIX 平台源文件配置
# =============================================================================
# 本文件由 core/osal/module.mk 自动包含
# 根据 Kconfig 细粒度配置编译对应的源文件

# 定义当前平台目录
osal_platform_dir := core/osal/src/posix

# 基础库文件（必需）
osal_SRCS += \
	$(osal_platform_dir)/lib/osal_errno.c \
	$(osal_platform_dir)/lib/osal_heap.c \
	$(osal_platform_dir)/lib/osal_stdio.c \
	$(osal_platform_dir)/lib/osal_string.c \
	$(osal_platform_dir)/util/osal_log.c \
	$(osal_platform_dir)/util/osal_version.c

# IPC 支持（细粒度控制）
ifeq ($(CONFIG_OSAL_IPC_ATOMIC),y)
osal_SRCS += $(osal_platform_dir)/ipc/osal_atomic.c
endif

ifeq ($(CONFIG_OSAL_IPC_COND),y)
osal_SRCS += $(osal_platform_dir)/ipc/osal_cond.c
endif

ifeq ($(CONFIG_OSAL_IPC_MUTEX),y)
osal_SRCS += $(osal_platform_dir)/ipc/osal_mutex.c
endif

ifeq ($(CONFIG_OSAL_IPC_SEMAPHORE),y)
osal_SRCS += $(osal_platform_dir)/ipc/osal_semaphore.c
endif

ifeq ($(CONFIG_OSAL_IPC_SHM),y)
osal_SRCS += $(osal_platform_dir)/ipc/osal_shm.c
endif

ifeq ($(CONFIG_OSAL_IPC_SHM_CACHE),y)
osal_SRCS += $(osal_platform_dir)/ipc/osal_shm_cache.c
endif

# 文件和系统支持（细粒度控制）
ifeq ($(CONFIG_OSAL_FILE_CLOCK),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_clock.c
endif

ifeq ($(CONFIG_OSAL_FILE_ENV),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_env.c
endif

ifeq ($(CONFIG_OSAL_FILE_FILE),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_file.c
endif

ifeq ($(CONFIG_OSAL_FILE_PROCESS),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_process.c
endif

ifeq ($(CONFIG_OSAL_FILE_SCHED),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_sched.c
endif

ifeq ($(CONFIG_OSAL_FILE_SELECT),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_select.c
endif

# 线程支持（细粒度控制）
ifeq ($(CONFIG_OSAL_THREAD_THREAD),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_thread.c
endif

ifeq ($(CONFIG_OSAL_THREAD_TIME),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_time.c
endif

# 信号支持（细粒度控制）
ifeq ($(CONFIG_OSAL_SIGNAL_SIGNAL),y)
osal_SRCS += $(osal_platform_dir)/sys/osal_signal.c
endif

# 网络支持（细粒度控制）
ifeq ($(CONFIG_OSAL_NETWORK_SOCKET),y)
osal_SRCS += $(osal_platform_dir)/net/osal_socket.c
endif

ifeq ($(CONFIG_OSAL_NETWORK_TERMIOS),y)
osal_SRCS += $(osal_platform_dir)/net/osal_termios.c
endif
