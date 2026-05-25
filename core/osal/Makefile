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
# 2. 确定 OS 实现目录
# -----------------------------------------------------------------------------
# 根据 OS 配置选择对应的源码目录
ifeq ($(CONFIG_OSAL_OS_POSIX),y)
  OSAL_OS_DIR := posix
else ifeq ($(CONFIG_OSAL_OS_WIN32),y)
  OSAL_OS_DIR := win32
else ifeq ($(CONFIG_OSAL_OS_RTOS),y)
  OSAL_OS_DIR := rtos
else ifeq ($(CONFIG_OSAL_OS_BARE),y)
  OSAL_OS_DIR := bare
else
  # 默认使用 POSIX
  OSAL_OS_DIR := posix
endif

# -----------------------------------------------------------------------------
# 3. 源文件列表（基础库）
# -----------------------------------------------------------------------------
# 通用源文件（所有平台共享）
osal-objs := \
	src/$(OSAL_OS_DIR)/lib/osal_errno.o \
	src/$(OSAL_OS_DIR)/lib/osal_heap.o \
	src/$(OSAL_OS_DIR)/lib/osal_stdio.o \
	src/$(OSAL_OS_DIR)/lib/osal_string.o \
	src/$(OSAL_OS_DIR)/util/osal_log.o \
	src/$(OSAL_OS_DIR)/util/osal_version.o

# -----------------------------------------------------------------------------
# 4. 条件编译源文件
# -----------------------------------------------------------------------------
# IPC 支持
osal-objs += $(if $(CONFIG_OSAL_IPC),src/$(OSAL_OS_DIR)/ipc/osal_atomic.o)
osal-objs += $(if $(CONFIG_OSAL_IPC),src/$(OSAL_OS_DIR)/ipc/osal_cond.o)
osal-objs += $(if $(CONFIG_OSAL_IPC),src/$(OSAL_OS_DIR)/ipc/osal_mutex.o)
osal-objs += $(if $(CONFIG_OSAL_IPC),src/$(OSAL_OS_DIR)/ipc/osal_semaphore.o)
osal-objs += $(if $(CONFIG_OSAL_IPC),src/$(OSAL_OS_DIR)/ipc/osal_shm.o)

# 文件和系统支持
osal-objs += $(if $(CONFIG_OSAL_FILE),src/$(OSAL_OS_DIR)/sys/osal_clock.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/$(OSAL_OS_DIR)/sys/osal_env.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/$(OSAL_OS_DIR)/sys/osal_file.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/$(OSAL_OS_DIR)/sys/osal_process.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/$(OSAL_OS_DIR)/sys/osal_sched.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/$(OSAL_OS_DIR)/sys/osal_select.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/$(OSAL_OS_DIR)/sys/osal_thread.o)
osal-objs += $(if $(CONFIG_OSAL_FILE),src/$(OSAL_OS_DIR)/sys/osal_time.o)

# 信号支持（仅 POSIX）
osal-objs += $(if $(CONFIG_OSAL_SIGNAL),src/$(OSAL_OS_DIR)/sys/osal_signal.o)

# 网络支持（POSIX 和 Win32）
osal-objs += $(if $(CONFIG_OSAL_NETWORK),src/$(OSAL_OS_DIR)/net/osal_socket.o)
osal-objs += $(if $(CONFIG_OSAL_NETWORK),src/$(OSAL_OS_DIR)/net/osal_termios.o)

# -----------------------------------------------------------------------------
# 5. 编译标志
# -----------------------------------------------------------------------------
# 通用编译标志（引用统一的 include 目录）
ccflags-y += -I$(srctree)/include/osal
ccflags-y += -I$(srctree)/include/osal/ipc
ccflags-y += -I$(srctree)/include/osal/lib
ccflags-y += -I$(srctree)/include/osal/net
ccflags-y += -I$(srctree)/include/osal/sys
ccflags-y += -I$(srctree)/include/osal/util

# 平台相关宏定义
ifeq ($(CONFIG_OSAL_OS_POSIX),y)
  ccflags-y += -DOSAL_OS_POSIX
endif
ifeq ($(CONFIG_OSAL_OS_WIN32),y)
  ccflags-y += -DOSAL_OS_WIN32
endif
ifeq ($(CONFIG_OSAL_OS_RTOS),y)
  ccflags-y += -DOSAL_OS_RTOS
endif
ifeq ($(CONFIG_OSAL_OS_BARE),y)
  ccflags-y += -DOSAL_OS_BARE
endif

# 架构相关宏定义
ifeq ($(CONFIG_OSAL_ARCH_32BIT),y)
  ccflags-y += -DOSAL_ARCH_32BIT
endif
ifeq ($(CONFIG_OSAL_ARCH_64BIT),y)
  ccflags-y += -DOSAL_ARCH_64BIT
endif

# 配置宏定义
ccflags-y += -DOSAL_MAX_TASKS=$(CONFIG_OSAL_MAX_TASKS)
ccflags-y += -DOSAL_MAX_QUEUES=$(CONFIG_OSAL_MAX_QUEUES)
ccflags-y += -DOSAL_MAX_MUTEXES=$(CONFIG_OSAL_MAX_MUTEXES)

# 动态库需要 -fPIC
osal-cflags-y := $(if $(CONFIG_OSAL_BUILD_SHARED),-fPIC)

# 调试标志
ifeq ($(CONFIG_OSAL_DEBUG_LOGGING),y)
  ccflags-y += -DOSAL_DEBUG_LOGGING
endif

# -----------------------------------------------------------------------------
# 6. 链接标志
# -----------------------------------------------------------------------------
# 动态库链接标志
osal-ldflags := $(if $(CONFIG_OSAL_BUILD_SHARED),-shared -Wl$(comma)-soname$(comma)libosal.so.1)

# POSIX 平台需要链接 pthread 和 rt
ifeq ($(CONFIG_OSAL_OS_POSIX),y)
  osal-ldflags += -lpthread -lrt
endif

# Win32 平台需要链接 ws2_32
ifeq ($(CONFIG_OSAL_OS_WIN32),y)
  osal-ldflags += -lws2_32
endif

# -----------------------------------------------------------------------------
# 7. 清理规则
# -----------------------------------------------------------------------------
clean-files := *.so *.so.* *.a
