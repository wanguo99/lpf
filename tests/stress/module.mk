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
	-Iinclude \
	 \
	 \
	 \
	

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
stress_test_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-ltestcore

# 根据启用的核心模块链接对应的库
# 注意：链接顺序很重要，依赖库要放在被依赖库之后
# 依赖关系：ACL -> PDL -> PCL -> HAL -> OSAL
# 使用 --no-as-needed 确保所有库都被链接

stress_test_LDFLAGS += -Wl,--no-as-needed

stress_test_LDFLAGS += -lacl

stress_test_LDFLAGS += -lpdl

stress_test_LDFLAGS += -lpcl

stress_test_LDFLAGS += -lhal

stress_test_LDFLAGS += -losal

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
# 声明依赖关系：测试程序依赖 testcore 和启用的核心模块共享库
# 注意：必须依赖 .so 文件而不是变量，确保共享库完全构建完成后才链接
$(stress_test_TARGET): $(stress_test_OBJS) $(STAGING_DIR)/lib/libtestcore.so
$(stress_test_TARGET): $(STAGING_DIR)/lib/libacl.so
$(stress_test_TARGET): $(STAGING_DIR)/lib/libpdl.so
$(stress_test_TARGET): $(STAGING_DIR)/lib/libpcl.so
$(stress_test_TARGET): $(STAGING_DIR)/lib/libhal.so
$(stress_test_TARGET): $(STAGING_DIR)/lib/libosal.so

$(stress_test_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(stress_test_OBJS) $(stress_test_LDFLAGS)
endif

# -----------------------------------------------------------------------------
# 8. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(stress_test_OBJS) $(stress_test_TARGET)
