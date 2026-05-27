# =============================================================================
# 系统测试构建配置（非递归 Make）
# =============================================================================
# 系统测试验证端到端功能和模块交互
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 源文件列表
# -----------------------------------------------------------------------------
system_test_SRCS :=

# 核心系统测试
ifeq ($(CONFIG_TEST_SYSTEM),y)
system_test_SRCS += tests/system/test_system_core.c
endif

# OSAL 系统测试
ifeq ($(CONFIG_TEST_SYSTEM_OSAL),y)
system_test_SRCS += tests/system/osal/test_system_osal.c
endif

# HAL 系统测试
ifeq ($(CONFIG_TEST_SYSTEM_HAL),y)
system_test_SRCS += tests/system/hal/test_system_hal.c
endif

# PCL 系统测试
ifeq ($(CONFIG_TEST_SYSTEM_PCL),y)
system_test_SRCS += tests/system/pcl/test_system_pcl.c
endif

# PDL 系统测试
ifeq ($(CONFIG_TEST_SYSTEM_PDL),y)
system_test_SRCS += tests/system/pdl/test_system_pdl.c
endif

# ACL 系统测试
ifeq ($(CONFIG_TEST_SYSTEM_ACL),y)
system_test_SRCS += tests/system/acl/test_system_acl.c
endif

# 集成测试
ifeq ($(CONFIG_TEST_INTEGRATION),y)
system_test_SRCS += tests/system/integration/test_system_integration.c
endif

# -----------------------------------------------------------------------------
# 2. 目标文件列表
# -----------------------------------------------------------------------------
system_test_OBJS := $(call srcs_to_objs,$(system_test_SRCS))

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
system_test_CFLAGS := \
	-Itests/include \
	-Iinclude \
	 \
	 \
	 \
	

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
system_test_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-ltestcore

# 根据启用的核心模块链接对应的库
# 注意：链接顺序很重要，依赖库要放在被依赖库之后
# 依赖关系：ACL -> PDL -> PCL -> HAL -> OSAL
# 使用 --no-as-needed 确保所有库都被链接

system_test_LDFLAGS += -Wl,--no-as-needed

system_test_LDFLAGS += -lacl

system_test_LDFLAGS += -lpdl

system_test_LDFLAGS += -lpcl

system_test_LDFLAGS += -lhal

system_test_LDFLAGS += -losal

system_test_LDFLAGS += -Wl,--as-needed -lpthread -lrt

# -----------------------------------------------------------------------------
# 5. 定义目标
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_TEST_SYSTEM),y)
system_test_TARGET := $(STAGING_DIR)/bin/ems-system-test
ALL_TARGETS += $(system_test_TARGET)
endif

# -----------------------------------------------------------------------------
# 6. 为此模块的目标文件添加编译标志
# -----------------------------------------------------------------------------
$(system_test_OBJS): CFLAGS += $(system_test_CFLAGS)


# -----------------------------------------------------------------------------
# 7. 定义构建规则
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_TEST_SYSTEM),y)
# 声明依赖关系：测试程序依赖 testcore 和启用的核心模块共享库
# 注意：必须依赖 .so 文件而不是变量，确保共享库完全构建完成后才链接
$(system_test_TARGET): $(system_test_OBJS) $(STAGING_DIR)/lib/libtestcore.so
$(system_test_TARGET): $(STAGING_DIR)/lib/libacl.so
$(system_test_TARGET): $(STAGING_DIR)/lib/libpdl.so
$(system_test_TARGET): $(STAGING_DIR)/lib/libpcl.so
$(system_test_TARGET): $(STAGING_DIR)/lib/libhal.so
$(system_test_TARGET): $(STAGING_DIR)/lib/libosal.so

$(system_test_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(system_test_OBJS) $(system_test_LDFLAGS)
endif

# -----------------------------------------------------------------------------
# 8. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(system_test_OBJS) $(system_test_TARGET)
