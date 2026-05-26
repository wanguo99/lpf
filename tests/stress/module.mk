# =============================================================================
# 压力测试构建配置（非递归 Make）
# =============================================================================
# 压力测试验证系统在高负载和极端条件下的行为
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 源文件列表
# -----------------------------------------------------------------------------
stress_test_SRCS :=

# 核心压力测试
ifeq ($(CONFIG_TEST_STRESS),y)
stress_test_SRCS += tests/stress/test_stress_core.c
endif

# OSAL 压力测试
ifeq ($(CONFIG_TEST_STRESS_OSAL),y)
stress_test_SRCS += tests/stress/osal/test_stress_osal.c
endif

# HAL 压力测试
ifeq ($(CONFIG_TEST_STRESS_HAL),y)
stress_test_SRCS += tests/stress/hal/test_stress_hal.c
endif

# PCL 压力测试
ifeq ($(CONFIG_TEST_STRESS_PCL),y)
stress_test_SRCS += tests/stress/pcl/test_stress_pcl.c
endif

# PDL 压力测试
ifeq ($(CONFIG_TEST_STRESS_PDL),y)
stress_test_SRCS += tests/stress/pdl/test_stress_pdl.c
endif

# ACL 压力测试
ifeq ($(CONFIG_TEST_STRESS_ACL),y)
stress_test_SRCS += tests/stress/acl/test_stress_acl.c
endif

# -----------------------------------------------------------------------------
# 2. 目标文件列表
# -----------------------------------------------------------------------------
stress_test_OBJS := $(call srcs_to_objs,$(stress_test_SRCS))

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
stress_test_CFLAGS := \
	-Itests/include \
	-Iinclude/osal \
	-Iinclude/hal \
	-Iinclude/pcl \
	-Iinclude/pdl \
	-Iinclude/acl

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
stress_test_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-ltestcore

# 根据启用的核心模块链接对应的库
ifeq ($(CONFIG_OSAL),y)
stress_test_LDFLAGS += -losal
endif

ifeq ($(CONFIG_HAL),y)
stress_test_LDFLAGS += -lhal
endif

ifeq ($(CONFIG_PCL),y)
stress_test_LDFLAGS += -lpcl
endif

ifeq ($(CONFIG_PDL),y)
stress_test_LDFLAGS += -lpdl
endif

ifeq ($(CONFIG_ACL),y)
stress_test_LDFLAGS += -lacl
endif

stress_test_LDFLAGS += -Wl,--as-needed -lpthread -lrt

# -----------------------------------------------------------------------------
# 5. 定义目标
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_TEST_STRESS),y)
stress_test_TARGET := $(STAGING_DIR)/bin/ems-stress-test
ALL_TARGETS += $(stress_test_TARGET)
endif

# -----------------------------------------------------------------------------
# 6. 为此模块的目标文件添加编译标志
# -----------------------------------------------------------------------------
$(stress_test_OBJS): CFLAGS += $(stress_test_CFLAGS)

# -----------------------------------------------------------------------------
# 7. 定义构建规则
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_TEST_STRESS),y)
# 声明依赖关系：测试程序依赖 testcore 和启用的核心模块库
$(stress_test_TARGET): $(stress_test_OBJS) $(testcore_TARGET)
ifeq ($(CONFIG_OSAL),y)
$(stress_test_TARGET): $(osal_TARGET)
endif
ifeq ($(CONFIG_HAL),y)
$(stress_test_TARGET): $(hal_TARGET)
endif
ifeq ($(CONFIG_PCL),y)
$(stress_test_TARGET): $(pcl_TARGET)
endif
ifeq ($(CONFIG_PDL),y)
$(stress_test_TARGET): $(pdl_TARGET)
endif
ifeq ($(CONFIG_ACL),y)
$(stress_test_TARGET): $(acl_TARGET)
endif

$(stress_test_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(stress_test_OBJS) $(stress_test_LDFLAGS)
endif

# -----------------------------------------------------------------------------
# 8. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(stress_test_OBJS) $(stress_test_TARGET)
