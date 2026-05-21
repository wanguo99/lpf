# ============================================================================
# OSAL 模块构建 - Linux 内核风格
# ============================================================================

# 包含 Kconfig 配置
-include $(objtree)/include/config/auto.conf

# 库名称
LIB_NAME := osal

# 根据 Kconfig 配置决定构建目标
ifeq ($(CONFIG_OSAL_BUILD_STATIC),y)
  obj-s := libosal.a
endif

ifeq ($(CONFIG_OSAL_BUILD_SHARED),y)
  obj-d := libosal.so
endif

# 对象文件列表（Linux 内核风格）
libosal.a-objs := \
	src/posix/lib/osal_errno.o \
	src/posix/lib/osal_heap.o \
	src/posix/lib/osal_stdio.o \
	src/posix/lib/osal_string.o \
	src/posix/util/osal_log.o \
	src/posix/util/osal_version.o

# IPC 支持（条件编译）
libosal.a-objs-$(CONFIG_OSAL_IPC) += \
	src/posix/ipc/osal_atomic.o \
	src/posix/ipc/osal_cond.o \
	src/posix/ipc/osal_mutex.o \
	src/posix/ipc/osal_semaphore.o \
	src/posix/ipc/osal_shm.o

# 文件和系统支持（条件编译）
libosal.a-objs-$(CONFIG_OSAL_FILE) += \
	src/posix/sys/osal_clock.o \
	src/posix/sys/osal_env.o \
	src/posix/sys/osal_file.o \
	src/posix/sys/osal_process.o \
	src/posix/sys/osal_sched.o \
	src/posix/sys/osal_select.o \
	src/posix/sys/osal_signal.o \
	src/posix/sys/osal_thread.o \
	src/posix/sys/osal_time.o

# 网络支持（条件编译）
libosal.a-objs-$(CONFIG_OSAL_NETWORK) += \
	src/posix/net/osal_socket.o \
	src/posix/net/osal_termios.o

# 动态库使用相同的对象文件
libosal.so-objs := $(libosal.a-objs)
libosal.so-objs-$(CONFIG_OSAL_IPC) := $(libosal.a-objs-y)
libosal.so-objs-$(CONFIG_OSAL_FILE) := $(libosal.a-objs-y)
libosal.so-objs-$(CONFIG_OSAL_NETWORK) := $(libosal.a-objs-y)

# 私有编译标志（每个模块独立定义）
CFLAGS_PRIV := -I$(srctree)/core/$(LIB_NAME)/include

# 私有链接标志和库（每个模块独立定义）
LDFLAGS_PRIV :=
LDLIBS_PRIV := -lpthread -lrt

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

# 包含统一构建框架
include $(scriptdir)/build/Makefile.build
