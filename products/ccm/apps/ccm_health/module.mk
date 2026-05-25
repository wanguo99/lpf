# =============================================================================
# ccm_health 应用程序非递归 Make 配置
# =============================================================================

ccm_health_SRCS := \
	products/ccm/apps/ccm_health/src/ccm_health.c \
	products/ccm/apps/ccm_health/src/main.c

ccm_health_CFLAGS := \
	-Iproducts/ccm/apps/ccm_health/include \
	-Iinclude/libccm \
	-Iinclude/acl \
	-Iinclude/osal

ccm_health_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-lh200_am625 -lccm -lacl -lpdl -lpcl -lhal -losal \
	-Wl,--as-needed -lpthread

ccm_health_OBJS := $(call srcs_to_objs,$(ccm_health_SRCS))

ifeq ($(CONFIG_BUILD_CCM_HEALTH),y)
ccm_health_TARGET := $(STAGING_DIR)/bin/ccm_health
ALL_TARGETS += $(ccm_health_TARGET)
endif

$(ccm_health_OBJS): CFLAGS += $(ccm_health_CFLAGS)

ifeq ($(CONFIG_BUILD_CCM_HEALTH),y)
$(ccm_health_TARGET): $(ccm_health_OBJS) $(STAGING_DIR)/lib/libh200_am625.so $(STAGING_DIR)/lib/libccm.so
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(ccm_health_OBJS) $(ccm_health_LDFLAGS)
endif

CLEAN_TARGETS += $(ccm_health_OBJS) $(ccm_health_TARGET)
