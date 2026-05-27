# =============================================================================
# ccm_collector 应用程序非递归 Make 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 基础源文件
# -----------------------------------------------------------------------------
ccm_collector_SRCS := \
	products/ccm/apps/ccm_collector/src/ccm_collector.c \
	products/ccm/apps/ccm_collector/src/main.c

# -----------------------------------------------------------------------------
# 2. 编译标志
# -----------------------------------------------------------------------------
ccm_collector_CFLAGS := \
	-Iproducts/ccm/apps/ccm_collector/include \
	 \
	-Iinclude \
	

# -----------------------------------------------------------------------------
# 3. 链接标志
# -----------------------------------------------------------------------------
ccm_collector_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-lh200_am625 \
	-lccm

# 根据启用的核心模块链接对应的库
ifeq ($(CONFIG_ACL),y)
ccm_collector_LDFLAGS += -lacl
endif

ifeq ($(CONFIG_PDL),y)
ccm_collector_LDFLAGS += -lpdl
endif

ifeq ($(CONFIG_PCL),y)
ccm_collector_LDFLAGS += -lpcl
endif

ifeq ($(CONFIG_HAL),y)
ccm_collector_LDFLAGS += -lhal
endif

ifeq ($(CONFIG_OSAL),y)
ccm_collector_LDFLAGS += -losal
endif

ccm_collector_LDFLAGS += -Wl,--as-needed -lpthread

# -----------------------------------------------------------------------------
# 4. 生成目标文件列表
# -----------------------------------------------------------------------------
ccm_collector_OBJS := $(call srcs_to_objs,$(ccm_collector_SRCS))

# -----------------------------------------------------------------------------
# 5. 定义目标
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_BUILD_CCM_COLLECTOR),y)
ccm_collector_TARGET := $(STAGING_DIR)/bin/ccm_collector
ALL_TARGETS += $(ccm_collector_TARGET)
endif

# -----------------------------------------------------------------------------
# 6. 添加编译标志到全局
# -----------------------------------------------------------------------------
$(ccm_collector_OBJS): CFLAGS += $(ccm_collector_CFLAGS)

ifeq ($(CONFIG_HAL),y)
endif
ifeq ($(CONFIG_PCL),y)
endif
ifeq ($(CONFIG_PDL),y)
endif
ifeq ($(CONFIG_ACL),y)
endif

# -----------------------------------------------------------------------------
# 7. 定义构建规则
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_BUILD_CCM_COLLECTOR),y)
# 声明依赖关系
$(ccm_collector_TARGET): $(ccm_collector_OBJS) $(STAGING_DIR)/lib/libh200_am625.so $(STAGING_DIR)/lib/libccm.so

ifeq ($(CONFIG_ACL),y)
$(ccm_collector_TARGET): $(STAGING_DIR)/lib/libacl.so
endif

ifeq ($(CONFIG_PDL),y)
$(ccm_collector_TARGET): $(STAGING_DIR)/lib/libpdl.so
endif

ifeq ($(CONFIG_PCL),y)
$(ccm_collector_TARGET): $(STAGING_DIR)/lib/libpcl.so
endif

ifeq ($(CONFIG_HAL),y)
$(ccm_collector_TARGET): $(STAGING_DIR)/lib/libhal.so
endif

ifeq ($(CONFIG_OSAL),y)
$(ccm_collector_TARGET): $(STAGING_DIR)/lib/libosal.so
endif

$(ccm_collector_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(ccm_collector_OBJS) $(ccm_collector_LDFLAGS)
endif

# -----------------------------------------------------------------------------
# 8. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(ccm_collector_OBJS) $(ccm_collector_TARGET)
