# =============================================================================
# ccm_health 应用程序非递归 Make 配置
# =============================================================================

ccm_health_SRCS := \
	products/ccm/apps/ccm_health/src/ccm_health.c \
	products/ccm/apps/ccm_health/src/main.c

ccm_health_CFLAGS := \
	-Iproducts/ccm/apps/ccm_health/include \
	 \
	-Iinclude \
	

ccm_health_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-lh200_am625 \
	-lccm

# 根据启用的核心模块链接对应的库
ifeq ($(CONFIG_ACL),y)
ccm_health_LDFLAGS += -lacl
endif

ifeq ($(CONFIG_PDL),y)
ccm_health_LDFLAGS += -lpdl
endif

ifeq ($(CONFIG_PCL),y)
ccm_health_LDFLAGS += -lpcl
endif

ifeq ($(CONFIG_HAL),y)
ccm_health_LDFLAGS += -lhal
endif

ifeq ($(CONFIG_OSAL),y)
ccm_health_LDFLAGS += -losal
endif

ccm_health_LDFLAGS += -Wl,--as-needed -lpthread

ccm_health_OBJS := $(call srcs_to_objs,$(ccm_health_SRCS))

ifeq ($(CONFIG_BUILD_CCM_HEALTH),y)
ccm_health_TARGET := $(STAGING_DIR)/bin/ccm_health
ALL_TARGETS += $(ccm_health_TARGET)
endif

$(ccm_health_OBJS): CFLAGS += $(ccm_health_CFLAGS)

ifeq ($(CONFIG_HAL),y)
endif
ifeq ($(CONFIG_PCL),y)
endif
ifeq ($(CONFIG_PDL),y)
endif
ifeq ($(CONFIG_ACL),y)
endif

ifeq ($(CONFIG_BUILD_CCM_HEALTH),y)
# 声明依赖关系
$(ccm_health_TARGET): $(ccm_health_OBJS) $(STAGING_DIR)/lib/libh200_am625.so $(STAGING_DIR)/lib/libccm.so

ifeq ($(CONFIG_ACL),y)
$(ccm_health_TARGET): $(STAGING_DIR)/lib/libacl.so
endif

ifeq ($(CONFIG_PDL),y)
$(ccm_health_TARGET): $(STAGING_DIR)/lib/libpdl.so
endif

ifeq ($(CONFIG_PCL),y)
$(ccm_health_TARGET): $(STAGING_DIR)/lib/libpcl.so
endif

ifeq ($(CONFIG_HAL),y)
$(ccm_health_TARGET): $(STAGING_DIR)/lib/libhal.so
endif

ifeq ($(CONFIG_OSAL),y)
$(ccm_health_TARGET): $(STAGING_DIR)/lib/libosal.so
endif

$(ccm_health_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(ccm_health_OBJS) $(ccm_health_LDFLAGS)
endif

CLEAN_TARGETS += $(ccm_health_OBJS) $(ccm_health_TARGET)
