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
# 2. 包含平台相关的源文件配置
# -----------------------------------------------------------------------------
# 每个平台维护自己的 module.mk，只需关注本平台的源文件
osal_SRCS :=
include core/osal/src/$(OSAL_OS_DIR)/module.mk

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
osal_CFLAGS := \
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

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
osal_LDFLAGS := -Wl,-soname,libosal.so.1

# POSIX 平台需要链接 pthread 和 rt
ifeq ($(CONFIG_OSAL_OS_POSIX),y)
  osal_LDFLAGS += -lpthread -lrt
endif

# Win32 平台需要链接 ws2_32
ifeq ($(CONFIG_OSAL_OS_WIN32),y)
  osal_LDFLAGS += -lws2_32
endif

# -----------------------------------------------------------------------------
# 5. 导出头文件
# -----------------------------------------------------------------------------
osal_HEADERS := \
	osal.h \
	osal_types.h \
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
# 以下为标准构建流程（无需修改）
# -----------------------------------------------------------------------------

# 生成目标文件列表
osal_OBJS := $(call srcs_to_objs,$(osal_SRCS))

# 定义库文件路径
ifeq ($(CONFIG_OSAL),y)
  ifeq ($(CONFIG_OSAL_BUILD_SHARED),y)
    osal_SO_TARGET := $(STAGING_DIR)/lib/libosal.so
    ALL_TARGETS += $(osal_SO_TARGET)
  endif

  ifeq ($(CONFIG_OSAL_BUILD_STATIC),y)
    osal_A_TARGET := $(STAGING_DIR)/lib/libosal.a
    ALL_TARGETS += $(osal_A_TARGET)
  endif
endif

# 添加编译标志
$(osal_OBJS): CFLAGS += $(osal_CFLAGS)

# 定义构建规则
ifeq ($(CONFIG_OSAL),y)

# 动态库构建
ifeq ($(CONFIG_OSAL_BUILD_SHARED),y)
$(osal_SO_TARGET): $(osal_OBJS)

$(osal_SO_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -shared -o $@ $(osal_OBJS) $(osal_LDFLAGS)
	@if [ -n "$(osal_LDFLAGS)" ] && echo "$(osal_LDFLAGS)" | grep -q "soname,"; then \
		soname=$$(echo "$(osal_LDFLAGS)" | sed -n 's/.*-soname,\([^ ]*\).*/\1/p'); \
		if [ -n "$$soname" ] && [ "$$soname" != "$$(basename $@)" ]; then \
			ln -sf $$(basename $@) $$(dirname $@)/$$soname; \
		fi; \
	fi
endif

# 静态库构建
ifeq ($(CONFIG_OSAL_BUILD_STATIC),y)
$(osal_A_TARGET): $(osal_OBJS)

$(osal_A_TARGET):
	@echo "  AR      $@"
	@mkdir -p $(dir $@)
	@rm -f $@
	@ar rcs $@ $(osal_OBJS)
endif

# 安装头文件到 staging 目录
ifneq ($(osal_HEADERS),)
$(osal_SO_TARGET) $(osal_A_TARGET): | install_osal_headers

.PHONY: install_osal_headers
install_osal_headers:
	@mkdir -p $(STAGING_DIR)/include/osal
	@for header in $(osal_HEADERS); do \
		src="include/osal/$$header"; \
		dst="$(STAGING_DIR)/include/osal/$$header"; \
		mkdir -p $$(dirname $$dst); \
		cp -f $$src $$dst; \
	done
endif

endif

# 清理规则
CLEAN_TARGETS += $(osal_OBJS) $(osal_SO_TARGET) $(osal_A_TARGET)
