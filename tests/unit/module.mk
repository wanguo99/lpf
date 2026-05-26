# =============================================================================
# 单元测试构建配置（非递归 Make）
# =============================================================================
# 单元测试验证各模块的独立功能
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 源文件列表
# -----------------------------------------------------------------------------
unit_test_SRCS :=

# OSAL 单元测试
ifeq ($(CONFIG_TEST_OSAL),y)
unit_test_SRCS += \
	tests/unit/osal/test_osal_atomic.c \
	tests/unit/osal/test_osal_clock.c \
	tests/unit/osal/test_osal_cond.c \
	tests/unit/osal/test_osal_env.c \
	tests/unit/osal/test_osal_errno.c \
	tests/unit/osal/test_osal_file.c \
	tests/unit/osal/test_osal_heap.c \
	tests/unit/osal/test_osal_log.c \
	tests/unit/osal/test_osal_mutex.c \
	tests/unit/osal/test_osal_process.c \
	tests/unit/osal/test_osal_sched.c \
	tests/unit/osal/test_osal_semaphore.c \
	tests/unit/osal/test_osal_shm.c \
	tests/unit/osal/test_osal_signal.c \
	tests/unit/osal/test_osal_stress.c \
	tests/unit/osal/test_osal_string.c \
	tests/unit/osal/test_osal_thread.c \
	tests/unit/osal/test_osal_time.c
endif

# HAL 单元测试
ifeq ($(CONFIG_TEST_HAL),y)
unit_test_SRCS += $(wildcard tests/unit/hal/*.c)
endif

# PCL 单元测试
ifeq ($(CONFIG_TEST_PCL),y)
unit_test_SRCS += $(wildcard tests/unit/pcl/*.c)
endif

# PDL 单元测试
ifeq ($(CONFIG_TEST_PDL),y)
# 排除缺少依赖的测试文件
unit_test_SRCS += $(filter-out tests/unit/pdl/test_pdl_bmc_protocol.c, $(wildcard tests/unit/pdl/*.c))
endif

# -----------------------------------------------------------------------------
# 2. 目标文件列表
# -----------------------------------------------------------------------------
unit_test_OBJS := $(call srcs_to_objs,$(unit_test_SRCS))

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
unit_test_CFLAGS := \
	-Itests/include \
	-Iinclude/osal \
	-Iinclude/hal \
	-Iinclude/pcl \
	-Iinclude/pdl

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
unit_test_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-ltestcore

# 根据启用的核心模块链接对应的库
ifeq ($(CONFIG_OSAL),y)
unit_test_LDFLAGS += -losal
endif

ifeq ($(CONFIG_HAL),y)
unit_test_LDFLAGS += -lhal
endif

ifeq ($(CONFIG_PCL),y)
unit_test_LDFLAGS += -lpcl
endif

ifeq ($(CONFIG_PDL),y)
unit_test_LDFLAGS += -lpdl
endif

ifeq ($(CONFIG_ACL),y)
unit_test_LDFLAGS += -lacl
endif

unit_test_LDFLAGS += -Wl,--as-needed -lpthread -lrt

# -----------------------------------------------------------------------------
# 5. 定义目标
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_TEST_UNIT),y)
unit_test_TARGET := $(STAGING_DIR)/bin/ems-unit-test
ALL_TARGETS += $(unit_test_TARGET)
endif

# -----------------------------------------------------------------------------
# 6. 为此模块的目标文件添加编译标志
# -----------------------------------------------------------------------------
$(unit_test_OBJS): CFLAGS += $(unit_test_CFLAGS)

# -----------------------------------------------------------------------------
# 7. 定义构建规则
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_TEST_UNIT),y)
# 声明依赖关系：测试程序依赖 testcore 和启用的核心模块库
$(unit_test_TARGET): $(unit_test_OBJS) $(testcore_TARGET)
ifeq ($(CONFIG_OSAL),y)
$(unit_test_TARGET): $(osal_TARGET)
endif
ifeq ($(CONFIG_HAL),y)
$(unit_test_TARGET): $(hal_TARGET)
endif
ifeq ($(CONFIG_PCL),y)
$(unit_test_TARGET): $(pcl_TARGET)
endif
ifeq ($(CONFIG_PDL),y)
$(unit_test_TARGET): $(pdl_TARGET)
endif
ifeq ($(CONFIG_ACL),y)
$(unit_test_TARGET): $(acl_TARGET)
endif

$(unit_test_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(unit_test_OBJS) $(unit_test_LDFLAGS)
endif

# -----------------------------------------------------------------------------
# 8. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(unit_test_OBJS) $(unit_test_TARGET)
