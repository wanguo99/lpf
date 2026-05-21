# =============================================================================
# PDL (Peripheral Driver Layer) 模块构建配置
# =============================================================================

# -----------------------------------------------------------------------------
# 1. 构建目标配置
# -----------------------------------------------------------------------------
# 根据 Kconfig 配置决定构建静态库或动态库

# 静态库
lib-$(CONFIG_PDL_BUILD_STATIC) += pdl

# 动态库
so-$(CONFIG_PDL_BUILD_SHARED) += pdl

# -----------------------------------------------------------------------------
# 2. 源文件列表
# -----------------------------------------------------------------------------
# 基础模块
pdl-objs := src/pdl_watchdog.o

# Satellite 支持（条件编译）
pdl-objs += $(if $(CONFIG_PDL_SATELLITE_SUPPORT),src/pdl_satellite/pdl_satellite.o)
pdl-objs += $(if $(CONFIG_PDL_SATELLITE_SUPPORT),src/pdl_satellite/pdl_satellite_can.o)

# BMC 支持（条件编译）
pdl-objs += $(if $(CONFIG_PDL_BMC_SUPPORT),src/pdl_bmc/pdl_bmc.o)
pdl-objs += $(if $(CONFIG_PDL_BMC_SUPPORT),src/pdl_bmc/pdl_bmc_ipmi.o)
pdl-objs += $(if $(CONFIG_PDL_BMC_SUPPORT),src/pdl_bmc/pdl_bmc_redfish.o)
pdl-objs += $(if $(CONFIG_PDL_BMC_SUPPORT),src/pdl_bmc/pdl_bmc_transport.o)

# MCU 支持（条件编译）
pdl-objs += $(if $(CONFIG_PDL_MCU_SUPPORT),src/pdl_mcu/pdl_mcu.o)
pdl-objs += $(if $(CONFIG_PDL_MCU_SUPPORT),src/pdl_mcu/pdl_mcu_can.o)
pdl-objs += $(if $(CONFIG_PDL_MCU_SUPPORT),src/pdl_mcu/pdl_mcu_protocol.o)
pdl-objs += $(if $(CONFIG_PDL_MCU_SUPPORT),src/pdl_mcu/pdl_mcu_serial.o)

# -----------------------------------------------------------------------------
# 3. 编译标志
# -----------------------------------------------------------------------------
# 通用编译标志
ccflags-y += -I$(src)/include
ccflags-y += -I$(srctree)/core/pcl/include
ccflags-y += -I$(srctree)/core/hal/include
ccflags-y += -I$(srctree)/core/osal/include
ccflags-y += -I$(srctree)/include

# 动态库需要 -fPIC
pdl-cflags-y := $(if $(CONFIG_PDL_BUILD_SHARED),-fPIC)

# 调试标志
ccflags-$(CONFIG_DEBUG_PDL) += -DDEBUG_PDL

# 特定模块的编译标志
ccflags-$(CONFIG_PDL_SATELLITE_SUPPORT) += -DPDL_SATELLITE_ENABLED
ccflags-$(CONFIG_PDL_BMC_SUPPORT) += -DPDL_BMC_ENABLED
ccflags-$(CONFIG_PDL_MCU_SUPPORT) += -DPDL_MCU_ENABLED

# -----------------------------------------------------------------------------
# 4. 链接标志
# -----------------------------------------------------------------------------
# 动态库链接标志
pdl-ldflags := $(if $(CONFIG_PDL_BUILD_SHARED),-shared -Wl$(comma)-soname$(comma)libpdl.so.1)
pdl-ldflags += -L$(objtree)/lib -lpcl -lhal -losal

# -----------------------------------------------------------------------------
# 5. 头文件导出
# -----------------------------------------------------------------------------
header-y := pdl.h pdl_satellite.h pdl_bmc.h pdl_mcu.h pdl_watchdog.h

# -----------------------------------------------------------------------------
# 6. 清理规则
# -----------------------------------------------------------------------------
clean-files := *.so *.so.* *.a
