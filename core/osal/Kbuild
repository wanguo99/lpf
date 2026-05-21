# =============================================================================
# OSAL (Operating System Abstraction Layer) 模块构建配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 构建目标配置
# -----------------------------------------------------------------------------
# 根据 Kconfig 配置决定构建静态库或动态库

# 静态库
lib-$(CONFIG_OSAL_BUILD_STATIC) += osal

# 动态库
so-$(CONFIG_OSAL_BUILD_SHARED) += osal

# -----------------------------------------------------------------------------
# 2. 源文件列表（基础库）
# -----------------------------------------------------------------------------
osal-objs := \
	src/posix/lib/osal_errno.o \
	src/posix/lib/osal_heap.o \
	src/posix/lib/osal_stdio.o \
	src/posix/lib/osal_string.o \
	src/posix/util/osal_log.o \
	src/posix/util/osal_version.o

# -----------------------------------------------------------------------------
# 3. 条件编译源文件
# -----------------------------------------------------------------------------
# IPC 支持
osal-objs += $(if $(CONFIG_OSAL_IPC),src/posix/ipc/osal_atomic.o)
osal-objs += $(if $(CONFIG_OSAL_IPC),src/posix/ipc/osal_cond.o)
osal-objs += $(if $(CONFIG_OSAL_IPC),src/posix/ipc/osal_mutex.o)
osal-objs += $(if $(CONFIG_OSAL_IPC),src/posix/ipc/osal_semaphore.o)
osal-objs += $(if $(CONFIG_OSAL_IPC),src/posix/ipc/osal_shm.o)

# 文件和系统支持
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_clock.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_env.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_file.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_process.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_sched.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_select.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_signal.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_thread.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/posix/sys/osal_time.o)

# 网络支持
osal-objs += $(if $(CONFIG_OSAL_NETWORK),src/posix/net/osal_socket.o)
osal-objs += $(if $(CONFIG_OSAL_NETWORK),src/posix/net/osal_termios.o)

# -----------------------------------------------------------------------------
# 4. 编译标志
# -----------------------------------------------------------------------------
# 通用编译标志
ccflags-y += -I$(src)/include

# 动态库需要 -fPIC
osal-cflags-y := $(if $(CONFIG_OSAL_BUILD_SHARED),-fPIC)

# 调试标志
ccflags-$(CONFIG_DEBUG_OSAL) += -DDEBUG_OSAL

# -----------------------------------------------------------------------------
# 5. 链接标志
# -----------------------------------------------------------------------------
# 动态库链接标志
osal-ldflags := $(if $(CONFIG_OSAL_BUILD_SHARED),-shared -Wl$(comma)-soname$(comma)libosal.so.1)
osal-ldflags += -lpthread -lrt

# -----------------------------------------------------------------------------
# 6. 头文件导出
# -----------------------------------------------------------------------------
header-y := \
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

# -----------------------------------------------------------------------------
# 7. 清理规则
# -----------------------------------------------------------------------------
clean-files := *.so *.so.* *.a
