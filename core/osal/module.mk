# =============================================================================
# OSAL 模块构建配置（非递归 Make）
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 确定 OS 实现目录
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_OSAL_OS_POSIX),y)
  OSAL_OS_DIR := posix
else ifeq ($(CONFIG_OSAL_OS_WIN32),y)
  OSAL_OS_DIR := win32
else ifeq ($(CONFIG_OSAL_OS_RTOS),y)
  OSAL_OS_DIR := rtos
else ifeq ($(CONFIG_OSAL_OS_BARE),y)
  OSAL_OS_DIR := bare
else
  OSAL_OS_DIR := posix
endif

# -----------------------------------------------------------------------------
# 2. 源文件列表
# -----------------------------------------------------------------------------
osal_SRCS := \
	core/osal/src/$(OSAL_OS_DIR)/lib/osal_errno.c \
	core/osal/src/$(OSAL_OS_DIR)/lib/osal_heap.c \
	core/osal/src/$(OSAL_OS_DIR)/lib/osal_stdio.c \
	core/osal/src/$(OSAL_OS_DIR)/lib/osal_string.c \
	core/osal/src/$(OSAL_OS_DIR)/util/osal_log.c \
	core/osal/src/$(OSAL_OS_DIR)/util/osal_version.c

# IPC 支持
ifeq ($(CONFIG_OSAL_IPC),y)
osal_SRCS += \
	core/osal/src/$(OSAL_OS_DIR)/ipc/osal_atomic.c \
	core/osal/src/$(OSAL_OS_DIR)/ipc/osal_cond.c \
	core/osal/src/$(OSAL_OS_DIR)/ipc/osal_mutex.c \
	core/osal/src/$(OSAL_OS_DIR)/ipc/osal_semaphore.c \
	core/osal/src/$(OSAL_OS_DIR)/ipc/osal_shm.c \
	core/osal/src/$(OSAL_OS_DIR)/ipc/osal_shm_cache.c
endif

# 文件和系统支持
ifeq ($(CONFIG_OSAL_FILE),y)
osal_SRCS += \
	core/osal/src/$(OSAL_OS_DIR)/sys/osal_clock.c \
	core/osal/src/$(OSAL_OS_DIR)/sys/osal_env.c \
	core/osal/src/$(OSAL_OS_DIR)/sys/osal_file.c \
	core/osal/src/$(OSAL_OS_DIR)/sys/osal_process.c \
	core/osal/src/$(OSAL_OS_DIR)/sys/osal_sched.c \
	core/osal/src/$(OSAL_OS_DIR)/sys/osal_select.c \
	core/osal/src/$(OSAL_OS_DIR)/sys/osal_thread.c \
	core/osal/src/$(OSAL_OS_DIR)/sys/osal_time.c
endif

# 信号支持
ifeq ($(CONFIG_OSAL_SIGNAL),y)
osal_SRCS += core/osal/src/$(OSAL_OS_DIR)/sys/osal_signal.c
endif

# 网络支持
ifeq ($(CONFIG_OSAL_NETWORK),y)
osal_SRCS += \
	core/osal/src/$(OSAL_OS_DIR)/net/osal_socket.c \
	core/osal/src/$(OSAL_OS_DIR)/net/osal_termios.c
endif

# -----------------------------------------------------------------------------
# 3. 目标文件列表
# -----------------------------------------------------------------------------
osal_OBJS := $(call srcs_to_objs,$(osal_SRCS))

# -----------------------------------------------------------------------------
# 4. 编译标志
# -----------------------------------------------------------------------------
osal_CFLAGS := \
	-Icore/osal/include \
	-Iinclude/osal \
	-Iinclude/osal/ipc \
	-Iinclude/osal/lib \
	-Iinclude/osal/net \
	-Iinclude/osal/sys \
	-Iinclude/osal/util

# 平台相关宏定义
ifeq ($(CONFIG_OSAL_OS_POSIX),y)
  osal_CFLAGS += -DOSAL_OS_POSIX
endif
ifeq ($(CONFIG_OSAL_OS_WIN32),y)
  osal_CFLAGS += -DOSAL_OS_WIN32
endif
ifeq ($(CONFIG_OSAL_OS_RTOS),y)
  osal_CFLAGS += -DOSAL_OS_RTOS
endif
ifeq ($(CONFIG_OSAL_OS_BARE),y)
  osal_CFLAGS += -DOSAL_OS_BARE
endif

# 架构相关宏定义
ifeq ($(CONFIG_OSAL_ARCH_32BIT),y)
  osal_CFLAGS += -DOSAL_ARCH_32BIT
endif
ifeq ($(CONFIG_OSAL_ARCH_64BIT),y)
  osal_CFLAGS += -DOSAL_ARCH_64BIT
endif

# 配置参数
osal_CFLAGS += -DOSAL_MAX_TASKS=$(CONFIG_OSAL_MAX_TASKS)
osal_CFLAGS += -DOSAL_MAX_QUEUES=$(CONFIG_OSAL_MAX_QUEUES)
osal_CFLAGS += -DOSAL_MAX_MUTEXES=$(CONFIG_OSAL_MAX_MUTEXES)

# 调试日志
ifeq ($(CONFIG_OSAL_DEBUG_LOGGING),y)
  osal_CFLAGS += -DOSAL_DEBUG_LOGGING
endif

# -----------------------------------------------------------------------------
# 5. 链接标志
# -----------------------------------------------------------------------------
osal_LDFLAGS := -Wl,-soname,libosal.so.1

# -----------------------------------------------------------------------------
# 6. 目标库
# -----------------------------------------------------------------------------
osal_TARGET := $(STAGING_DIR)/lib/libosal.so
osal_STATIC := $(STAGING_DIR)/lib/libosal.a

# 添加到全局目标列表
ALL_TARGETS += $(osal_TARGET)

# -----------------------------------------------------------------------------
# 7. 构建规则
# -----------------------------------------------------------------------------
$(eval $(call build_shared_lib,$(osal_TARGET),$(osal_OBJS),$(osal_LDFLAGS)))
$(eval $(call build_static_lib,$(osal_STATIC),$(osal_OBJS)))

# 为此模块的目标文件添加编译标志
$(osal_OBJS): CFLAGS += $(osal_CFLAGS)
