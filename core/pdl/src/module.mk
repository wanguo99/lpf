# =============================================================================
# PDL 源文件配置
# =============================================================================

# 定义子模块目录
pdl_src_dir := core/pdl/src

# Watchdog 支持（细粒度控制）
ifeq ($(CONFIG_PDL_WATCHDOG),y)
pdl_SRCS += $(pdl_src_dir)/pdl_watchdog.c
endif

# Satellite 支持（细粒度控制）
ifeq ($(CONFIG_PDL_SATELLITE),y)
pdl_SRCS += $(pdl_src_dir)/pdl_satellite/pdl_satellite.c
endif

ifeq ($(CONFIG_PDL_SATELLITE_CAN),y)
pdl_SRCS += $(pdl_src_dir)/pdl_satellite/pdl_satellite_can.c
endif

# BMC 支持（细粒度控制）
ifeq ($(CONFIG_PDL_BMC),y)
pdl_SRCS += $(pdl_src_dir)/pdl_bmc/pdl_bmc.c
endif

ifeq ($(CONFIG_PDL_BMC_IPMI),y)
pdl_SRCS += $(pdl_src_dir)/pdl_bmc/pdl_bmc_ipmi.c
endif

ifeq ($(CONFIG_PDL_BMC_REDFISH),y)
pdl_SRCS += $(pdl_src_dir)/pdl_bmc/pdl_bmc_redfish.c
endif

ifeq ($(CONFIG_PDL_BMC_TRANSPORT),y)
pdl_SRCS += $(pdl_src_dir)/pdl_bmc/pdl_bmc_transport.c
endif

# MCU 支持（细粒度控制）
ifeq ($(CONFIG_PDL_MCU),y)
pdl_SRCS += $(pdl_src_dir)/pdl_mcu/pdl_mcu.c
endif

ifeq ($(CONFIG_PDL_MCU_CAN),y)
pdl_SRCS += $(pdl_src_dir)/pdl_mcu/pdl_mcu_can.c
endif

ifeq ($(CONFIG_PDL_MCU_PROTOCOL),y)
pdl_SRCS += $(pdl_src_dir)/pdl_mcu/pdl_mcu_protocol.c
endif

ifeq ($(CONFIG_PDL_MCU_SERIAL),y)
pdl_SRCS += $(pdl_src_dir)/pdl_mcu/pdl_mcu_serial.c
endif
