# =============================================================================
# OSAL RTOS 平台源文件配置
# =============================================================================
# 本文件由 core/osal/module.mk 自动包含
# 只需维护本平台的源文件列表

# 基础库文件
osal_SRCS += \
	core/osal/src/rtos/lib/osal_errno.c \
	core/osal/src/rtos/lib/osal_heap.c

# IPC 支持
ifeq ($(CONFIG_OSAL_IPC),y)
osal_SRCS += \
	core/osal/src/rtos/ipc/osal_mutex.c \
	core/osal/src/rtos/ipc/osal_semaphore.c \
	core/osal/src/rtos/ipc/osal_queue.c
endif

# 线程支持
ifeq ($(CONFIG_OSAL_THREAD),y)
osal_SRCS += \
	core/osal/src/rtos/sys/osal_thread.c \
	core/osal/src/rtos/sys/osal_time.c
endif
