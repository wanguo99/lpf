# =============================================================================
# ccm_logger 应用程序非递归 Make 配置
# =============================================================================

ccm_logger_SRCS := \
	products/ccm/apps/ccm_logger/src/ccm_logger.c \
	products/ccm/apps/ccm_logger/src/main.c

ccm_logger_CFLAGS := \
	-Iproducts/ccm/apps/ccm_logger/include \
	 \
	-Iinclude \
	

ccm_logger_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-lccm

# 根据启用的核心模块链接对应的库
ifeq ($(CONFIG_OSAL),y)
ccm_logger_LDFLAGS += -losal
endif

ccm_logger_LDFLAGS += -Wl,--as-needed -lpthread

ccm_logger_OBJS := $(call srcs_to_objs,$(ccm_logger_SRCS))

ifeq ($(CONFIG_BUILD_CCM_LOGGER),y)
ccm_logger_TARGET := $(STAGING_DIR)/bin/ccm_logger
ALL_TARGETS += $(ccm_logger_TARGET)
endif

$(ccm_logger_OBJS): CFLAGS += $(ccm_logger_CFLAGS)


ifeq ($(CONFIG_BUILD_CCM_LOGGER),y)
# 声明依赖关系
$(ccm_logger_TARGET): $(ccm_logger_OBJS) $(STAGING_DIR)/lib/libccm.so

ifeq ($(CONFIG_OSAL),y)
$(ccm_logger_TARGET): $(STAGING_DIR)/lib/libosal.so
endif

$(ccm_logger_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(ccm_logger_OBJS) $(ccm_logger_LDFLAGS)
endif

CLEAN_TARGETS += $(ccm_logger_OBJS) $(ccm_logger_TARGET)
