# =============================================================================
# PDL 源文件配置
# =============================================================================

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
