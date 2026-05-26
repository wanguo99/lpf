# =============================================================================
# ccm_supervisor 应用程序非递归 Make 配置
# =============================================================================

ccm_supervisor_SRCS := \
	products/ccm/apps/ccm_supervisor/src/ccm_supervisor.c \
	products/ccm/apps/ccm_supervisor/src/main.c

ccm_supervisor_CFLAGS := \
	-Iproducts/ccm/apps/ccm_supervisor/include \
	-Iinclude/libccm \
	-Iinclude/acl \
	-Iinclude/osal

ccm_supervisor_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-lccm

# 根据启用的核心模块链接对应的库
ifeq ($(CONFIG_OSAL),y)
ccm_supervisor_LDFLAGS += -losal
endif

ccm_supervisor_LDFLAGS += -Wl,--as-needed -lpthread

ccm_supervisor_OBJS := $(call srcs_to_objs,$(ccm_supervisor_SRCS))

ifeq ($(CONFIG_BUILD_CCM_SUPERVISOR),y)
ccm_supervisor_TARGET := $(STAGING_DIR)/bin/ccm_supervisor
ALL_TARGETS += $(ccm_supervisor_TARGET)
endif

$(ccm_supervisor_OBJS): CFLAGS += $(ccm_supervisor_CFLAGS)

ifeq ($(CONFIG_BUILD_CCM_SUPERVISOR),y)
# 声明依赖关系
$(ccm_supervisor_TARGET): $(ccm_supervisor_OBJS) $(STAGING_DIR)/lib/libccm.so

ifeq ($(CONFIG_OSAL),y)
$(ccm_supervisor_TARGET): $(STAGING_DIR)/lib/libosal.so
endif

$(ccm_supervisor_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(ccm_supervisor_OBJS) $(ccm_supervisor_LDFLAGS)
endif

CLEAN_TARGETS += $(ccm_supervisor_OBJS) $(ccm_supervisor_TARGET)
