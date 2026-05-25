# =============================================================================
# PDL 模块非递归 Make 配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 基础源文件
# -----------------------------------------------------------------------------
pdl_SRCS :=

# Watchdog 支持
ifeq ($(CONFIG_PDL_WATCHDOG_SUPPORT),y)
pdl_SRCS += core/pdl/src/pdl_watchdog.c
endif

# Satellite 支持
ifeq ($(CONFIG_PDL_SATELLITE_SUPPORT),y)
pdl_SRCS += \
	core/pdl/src/pdl_satellite/pdl_satellite.c \
	core/pdl/src/pdl_satellite/pdl_satellite_can.c
endif

# BMC 支持
ifeq ($(CONFIG_PDL_BMC_SUPPORT),y)
pdl_SRCS += \
	core/pdl/src/pdl_bmc/pdl_bmc.c \
	core/pdl/src/pdl_bmc/pdl_bmc_ipmi.c \
	core/pdl/src/pdl_bmc/pdl_bmc_redfish.c \
	core/pdl/src/pdl_bmc/pdl_bmc_transport.c
endif

# MCU 支持
ifeq ($(CONFIG_PDL_MCU_SUPPORT),y)
pdl_SRCS += \
	core/pdl/src/pdl_mcu/pdl_mcu.c \
	core/pdl/src/pdl_mcu/pdl_mcu_can.c \
	core/pdl/src/pdl_mcu/pdl_mcu_protocol.c \
	core/pdl/src/pdl_mcu/pdl_mcu_serial.c
endif

# -----------------------------------------------------------------------------
# 2. 编译标志
# -----------------------------------------------------------------------------
pdl_CFLAGS := \
	-Iinclude/pdl \
	-Iinclude/pcl \
	-Iinclude/pcl/api \
	-Iinclude/hal \
	-Iinclude/osal

# -----------------------------------------------------------------------------
# 3. 链接标志
# -----------------------------------------------------------------------------
pdl_LDFLAGS := \
	-L$(STAGING_DIR)/lib \
	-Wl,--no-as-needed \
	-lpcl \
	-lhal \
	-losal \
	-Wl,--as-needed \
	-lpthread

# -----------------------------------------------------------------------------
# 4. 生成目标文件列表
# -----------------------------------------------------------------------------
pdl_OBJS := $(call srcs_to_objs,$(pdl_SRCS))

# -----------------------------------------------------------------------------
# 5. 定义目标
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_PDL_BUILD_SHARED),y)
pdl_TARGET := $(STAGING_DIR)/lib/libpdl.so
ALL_TARGETS += $(pdl_TARGET)
endif

ifeq ($(CONFIG_PDL_BUILD_STATIC),y)
pdl_STATIC_TARGET := $(STAGING_DIR)/lib/libpdl.a
ALL_TARGETS += $(pdl_STATIC_TARGET)
endif

# -----------------------------------------------------------------------------
# 6. 添加编译标志到全局
# -----------------------------------------------------------------------------
$(pdl_OBJS): CFLAGS += $(pdl_CFLAGS)

# -----------------------------------------------------------------------------
# 7. 定义构建规则
# -----------------------------------------------------------------------------
ifeq ($(CONFIG_PDL_BUILD_SHARED),y)
$(eval $(call build_shared_lib,$(pdl_TARGET),$(pdl_OBJS),$(pdl_LDFLAGS)))
endif

ifeq ($(CONFIG_PDL_BUILD_STATIC),y)
$(eval $(call build_static_lib,$(pdl_STATIC_TARGET),$(pdl_OBJS)))
endif

# -----------------------------------------------------------------------------
# 8. 依赖关系
# -----------------------------------------------------------------------------
# pdl 依赖 pcl, hal, osal
$(pdl_TARGET): $(STAGING_DIR)/lib/libpcl.so $(STAGING_DIR)/lib/libhal.so $(STAGING_DIR)/lib/libosal.so
$(pdl_STATIC_TARGET): $(STAGING_DIR)/lib/libpcl.so $(STAGING_DIR)/lib/libhal.so $(STAGING_DIR)/lib/libosal.so

# -----------------------------------------------------------------------------
# 9. 清理规则
# -----------------------------------------------------------------------------
CLEAN_TARGETS += $(pdl_OBJS) $(pdl_TARGET) $(pdl_STATIC_TARGET)
