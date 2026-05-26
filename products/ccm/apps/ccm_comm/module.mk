# =============================================================================
# ccm_comm 应用程序非递归 Make 配置
# =============================================================================

ccm_comm_SRCS := \
	products/ccm/apps/ccm_comm/src/ccm_comm.c \
	products/ccm/apps/ccm_comm/src/main.c

ccm_comm_CFLAGS := \
	-Iproducts/ccm/apps/ccm_comm/include \
	-Iinclude/libccm \
	-Iinclude/acl \
	-Iinclude/osal

ccm_comm_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-lh200_am625 \
	-lccm

# 根据启用的核心模块链接对应的库
ifeq ($(CONFIG_ACL),y)
ccm_comm_LDFLAGS += -lacl
endif

ifeq ($(CONFIG_PDL),y)
ccm_comm_LDFLAGS += -lpdl
endif

ifeq ($(CONFIG_PCL),y)
ccm_comm_LDFLAGS += -lpcl
endif

ifeq ($(CONFIG_HAL),y)
ccm_comm_LDFLAGS += -lhal
endif

ifeq ($(CONFIG_OSAL),y)
ccm_comm_LDFLAGS += -losal
endif

ccm_comm_LDFLAGS += -Wl,--as-needed -lpthread

ccm_comm_OBJS := $(call srcs_to_objs,$(ccm_comm_SRCS))

ifeq ($(CONFIG_BUILD_CCM_COMM),y)
ccm_comm_TARGET := $(STAGING_DIR)/bin/ccm_comm
ALL_TARGETS += $(ccm_comm_TARGET)
endif

$(ccm_comm_OBJS): CFLAGS += $(ccm_comm_CFLAGS)

ifeq ($(CONFIG_BUILD_CCM_COMM),y)
# 声明依赖关系
$(ccm_comm_TARGET): $(ccm_comm_OBJS) $(STAGING_DIR)/lib/libh200_am625.so $(STAGING_DIR)/lib/libccm.so

ifeq ($(CONFIG_ACL),y)
$(ccm_comm_TARGET): $(STAGING_DIR)/lib/libacl.so
endif

ifeq ($(CONFIG_PDL),y)
$(ccm_comm_TARGET): $(STAGING_DIR)/lib/libpdl.so
endif

ifeq ($(CONFIG_PCL),y)
$(ccm_comm_TARGET): $(STAGING_DIR)/lib/libpcl.so
endif

ifeq ($(CONFIG_HAL),y)
$(ccm_comm_TARGET): $(STAGING_DIR)/lib/libhal.so
endif

ifeq ($(CONFIG_OSAL),y)
$(ccm_comm_TARGET): $(STAGING_DIR)/lib/libosal.so
endif

$(ccm_comm_TARGET):
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	@$(CC) -o $@ $(ccm_comm_OBJS) $(ccm_comm_LDFLAGS)
endif

CLEAN_TARGETS += $(ccm_comm_OBJS) $(ccm_comm_TARGET)
