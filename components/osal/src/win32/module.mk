# =============================================================================
# OSAL Win32 平台源文件配置
# =============================================================================
# 本文件由 core/osal/module.mk 自动包含
# 只需维护本平台的源文件列表

# 基础库文件
osal_SRCS += \
	core/osal/src/win32/lib/osal_errno.c \
	core/osal/src/win32/lib/osal_heap.c \
	core/osal/src/win32/lib/osal_stdio.c \
	core/osal/src/win32/lib/osal_string.c

# IPC 支持
ifeq ($(CONFIG_OSAL_IPC),y)
osal_SRCS += \
	core/osal/src/win32/ipc/osal_mutex.c \
	core/osal/src/win32/ipc/osal_semaphore.c
endif

# 文件和系统支持
ifeq ($(CONFIG_OSAL_FILE),y)
osal_SRCS += \
	core/osal/src/win32/sys/osal_file.c \
	core/osal/src/win32/sys/osal_thread.c \
	core/osal/src/win32/sys/osal_time.c
endif

# 网络支持
ifeq ($(CONFIG_OSAL_NETWORK),y)
osal_SRCS += \
	core/osal/src/win32/net/osal_socket.c
endif
