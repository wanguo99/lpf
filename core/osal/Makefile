# ============================================================================
# OSAL 模块构建
# ============================================================================

# 必须在包含 Makefile.lib 之前定义
GENERATED_DIR ?= $(objtree)/include/generated

include $(srctree)/scripts/Makefile.lib

# 模块名称
MODULE := osal
LIB_NAME := libosal.so

# 输出目录
BUILD_DIR ?= $(objtree)/core/$(MODULE)
LIB_DIR := $(objtree)/lib

# 源文件目录
SRC_DIR := $(srctree)/core/$(MODULE)/src/posix
INC_DIR := $(srctree)/core/$(MODULE)/include

# 源文件列表
SRCS := \
	$(SRC_DIR)/lib/osal_errno.c \
	$(SRC_DIR)/ipc/osal_mutex.c \
	$(SRC_DIR)/ipc/osal_atomic.c \
	$(SRC_DIR)/ipc/osal_semaphore.c \
	$(SRC_DIR)/ipc/osal_cond.c \
	$(SRC_DIR)/ipc/osal_shm.c \
	$(SRC_DIR)/sys/osal_clock.c \
	$(SRC_DIR)/sys/osal_signal.c \
	$(SRC_DIR)/sys/osal_file.c \
	$(SRC_DIR)/sys/osal_select.c \
	$(SRC_DIR)/sys/osal_env.c \
	$(SRC_DIR)/sys/osal_time.c \
	$(SRC_DIR)/sys/osal_process.c \
	$(SRC_DIR)/sys/osal_thread.c \
	$(SRC_DIR)/sys/osal_sched.c \
	$(SRC_DIR)/net/osal_socket.c \
	$(SRC_DIR)/net/osal_termios.c \
	$(SRC_DIR)/lib/osal_heap.c \
	$(SRC_DIR)/lib/osal_string.c \
	$(SRC_DIR)/lib/osal_stdio.c \
	$(SRC_DIR)/util/osal_version.c \
	$(SRC_DIR)/util/osal_log.c

# 目标文件（放在 BUILD_DIR 中）
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# 模块特定的编译标志
CFLAGS_MODULE := -I$(INC_DIR)

# 链接库
LDLIBS := -lpthread
ifeq ($(shell uname -s),Linux)
  LDLIBS += -lrt
endif

# ============================================================================
# 构建目标
# ============================================================================

.PHONY: all clean

all: $(LIB_DIR)/$(LIB_NAME)

$(LIB_DIR)/$(LIB_NAME): $(OBJS) | $(LIB_DIR)
	$(call cmd,ld_so)

# 编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(call cmd,cc_o_c)

# 创建输出目录
$(LIB_DIR):
	@mkdir -p $@

# 包含依赖文件（自动跟踪头文件变化）
-include $(OBJS:.o=.d)

clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(LIB_DIR)/$(LIB_NAME)
