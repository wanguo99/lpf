# ============================================================================
# OSAL 模块构建 - 使用通用库构建框架
# ============================================================================

# 库配置
LIB_NAME := osal
# LIB_TYPE := static  # 取消注释可覆盖全局配置

# 源文件目录（POSIX 实现）
SRC_DIR := $(srctree)/core/$(LIB_NAME)/src/posix

# 源文件列表（相对于 SRC_DIR）
SRCS := \
	lib/osal_errno.c \
	lib/osal_heap.c \
	lib/osal_string.c \
	lib/osal_stdio.c \
	ipc/osal_mutex.c \
	ipc/osal_atomic.c \
	ipc/osal_semaphore.c \
	ipc/osal_cond.c \
	ipc/osal_shm.c \
	sys/osal_clock.c \
	sys/osal_signal.c \
	sys/osal_file.c \
	sys/osal_select.c \
	sys/osal_env.c \
	sys/osal_time.c \
	sys/osal_process.c \
	sys/osal_thread.c \
	sys/osal_sched.c \
	net/osal_socket.c \
	net/osal_termios.c \
	util/osal_version.c \
	util/osal_log.c

# 导出的头文件（相对于 INC_DIR，会被复制到顶层 include/osal/）
EXPORT_HEADERS := \
	osal.h \
	osal_types.h \
	osal_platform.h \
	ipc/osal_atomic.h \
	ipc/osal_cond.h \
	ipc/osal_mutex.h \
	ipc/osal_semaphore.h \
	ipc/osal_shm.h \
	lib/osal_errno.h \
	lib/osal_heap.h \
	lib/osal_stdio.h \
	lib/osal_string.h \
	net/osal_socket.h \
	net/osal_termios.h \
	sys/osal_clock.h \
	sys/osal_env.h \
	sys/osal_file.h \
	sys/osal_process.h \
	sys/osal_sched.h \
	sys/osal_select.h \
	sys/osal_signal.h \
	sys/osal_thread.h \
	sys/osal_time.h \
	util/osal_log.h \
	util/osal_version.h

# 链接库
LDLIBS := -lpthread
ifeq ($(shell uname -s),Linux)
  LDLIBS += -lrt
endif

# 包含通用库构建框架
include $(srctree)/scripts/Makefile.lib
