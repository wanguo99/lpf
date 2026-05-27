# =============================================================================
# 性能测试构建配置（非递归 Make）
# =============================================================================
# 性能测试测量吞吐量、延迟和资源使用
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 源文件列表
# -----------------------------------------------------------------------------
perf_test_SRCS :=

# 核心性能测试
ifeq ($(CONFIG_TEST_PERFORMANCE),y)
perf_test_SRCS += tests/performance/test_perf_core.c
endif

# OSAL 性能测试
ifeq ($(CONFIG_TEST_PERFORMANCE_OSAL),y)
perf_test_SRCS += tests/performance/osal/test_perf_osal.c
endif

# HAL 性能测试
ifeq ($(CONFIG_TEST_PERFORMANCE_HAL),y)
perf_test_SRCS += tests/performance/hal/test_perf_hal.c
endif

# PCL 性能测试
ifeq ($(CONFIG_TEST_PERFORMANCE_PCL),y)
perf_test_SRCS += tests/performance/pcl/test_perf_pcl.c
endif

# PDL 性能测试
ifeq ($(CONFIG_TEST_PERFORMANCE_PDL),y)
perf_test_SRCS += tests/performance/pdl/test_perf_pdl.c
endif

# ACL 性能测试
ifeq ($(CONFIG_TEST_PERFORMANCE_ACL),y)
perf_test_SRCS += tests/performance/acl/test_perf_acl.c
endif

# -----------------------------------------------------------------------------
# 2. 目标文件列表
# -----------------------------------------------------------------------------
perf_test_OBJS := $(call srcs_to_objs,$(perf_test_SRCS))

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
perf_test_CFLAGS := \
	-Itests/include \
	-Iinclude \
	 \
	 \
	 \
	

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
perf_test_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-ltestcore

# 根据启用的核心模块链接对应的库
# 注意：链接顺序很重要，依赖库要放在被依赖库之后
# 依赖关系：ACL -> PDL -> PCL -> HAL -> OSAL
# 使用 --no-as-needed 确保所有库都被链接

perf_test_LDFLAGS += -Wl,--no-as-needed

ifeq ($(CONFIG_ACL),y)
perf_test_LDFLAGS += -lacl
endif

ifeq ($(CONFIG_PDL),y)
perf_test_LDFLAGS += -lpdl
endif

ifeq ($(CONFIG_PCL),y)
perf_test_LDFLAGS += -lpcl
endif

ifeq ($(CONFIG_HAL),y)
perf_test_LDFLAGS += -lhal
endif

ifeq ($(CONFIG_OSAL),y)
perf_test_LDFLAGS += -losal
endif

perf_test_LDFLAGS += -Wl,--as-needed -lpthread -lrt

# -----------------------------------------------------------------------------
# 5. 定义目标
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_TEST_PERFORMANCE),y)
perf_test_TARGET := $(STAGING_DIR)/bin/ems-perf-test
ALL_TARGETS += $(perf_test_TARGET)
endif

# -----------------------------------------------------------------------------
# 6. 为此模块的目标文件添加编译标志
# -----------------------------------------------------------------------------
$(perf_test_OBJS): CFLAGS += $(perf_test_CFLAGS)

# 确保在所有依赖库的头文件安装后才编译
$(perf_test_OBJS): | $(STAGING_DIR)/lib/libtestcore.so
ifeq ($(CONFIG_OSAL),y)
$(perf_test_OBJS): | $(STAGING_DIR)/lib/libosal.so
endif
ifeq ($(CONFIG_HAL),y)
$(perf_test_OBJS): | $(STAGING_DIR)/lib/libhal.so
endif
ifeq ($(CONFIG_PCL),y)
$(perf_test_OBJS): | $(STAGING_DIR)/lib/libpcl.so
endif
ifeq ($(CONFIG_PDL),y)
$(perf_test_OBJS): | $(STAGING_DIR)/lib/libpdl.so
endif
ifeq ($(CONFIG_ACL),y)
$(perf_test_OBJS): | $(STAGING_DIR)/lib/libacl.so
endif

# -----------------------------------------------------------------------------
# 7. 定义构建规则
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_TEST_PERFORMANCE),y)
# 声明依赖关系：测试程序依赖 testcore 和启用的核心模块共享库
# 注意：必须依赖 .so 文件而不是变量，确保共享库完全构建完成后才链接
$(perf_test_TARGET): $(perf_test_OBJS) $(STAGING_DIR)/lib/libtestcore.so
ifeq ($(CONFIG_ACL),y)
$(perf_test_TARGET): $(STAGING_DIR)/lib/libacl.so
endif
ifeq ($(CONFIG_PDL),y)
$(perf_test_TARGET): $(STAGING_DIR)/lib/libpdl.so
endif
ifeq ($(CONFIG_PCL),y)
$(perf_test_TARGET): $(STAGING_DIR)/lib/libpcl.so
endif
ifeq ($(CONFIG_HAL),y)
$(perf_test_TARGET): $(STAGING_DIR)/lib/libhal.so
endif
ifeq ($(CONFIG_OSAL),y)
$(perf_test_TARGET): $(STAGING_DIR)/lib/libosal.so
endif

$(perf_test_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(perf_test_OBJS) $(perf_test_LDFLAGS)
endif

# -----------------------------------------------------------------------------
# 8. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(perf_test_OBJS) $(perf_test_TARGET)
