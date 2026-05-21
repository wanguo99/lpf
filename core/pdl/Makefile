# ============================================================================
# PDL 模块构建 - Kbuild 风格
# ============================================================================

# 包含 Kconfig 配置
-include $(objtree)/include/config/auto.conf

# 库名称
LIB_NAME := pdl

# 根据 Kconfig 配置决定构建目标
ifeq ($(CONFIG_PDL_BUILD_STATIC),y)
  obj-s := libpdl.a
endif

ifeq ($(CONFIG_PDL_BUILD_SHARED),y)
  obj-d := libpdl.so
endif

# 对象文件列表（Linux 内核风格）
libpdl.a-objs-y := src/pdl_watchdog.o

# Satellite 支持（条件编译）
libpdl.a-objs-$(CONFIG_PDL_SATELLITE_SUPPORT) += \
	src/pdl_satellite/pdl_satellite.o \
	src/pdl_satellite/pdl_satellite_can.o

# BMC 支持（条件编译）
libpdl.a-objs-$(CONFIG_PDL_BMC_SUPPORT) += \
	src/pdl_bmc/pdl_bmc.o \
	src/pdl_bmc/pdl_bmc_ipmi.o \
	src/pdl_bmc/pdl_bmc_redfish.o \
	src/pdl_bmc/pdl_bmc_transport.o

# MCU 支持（条件编译）
libpdl.a-objs-$(CONFIG_PDL_MCU_SUPPORT) += \
	src/pdl_mcu/pdl_mcu.o \
	src/pdl_mcu/pdl_mcu_can.o \
	src/pdl_mcu/pdl_mcu_protocol.o \
	src/pdl_mcu/pdl_mcu_serial.o

# 动态库使用相同的对象文件
libpdl.so-objs-y := $(libpdl.a-objs-y)
libpdl.so-objs-$(CONFIG_PDL_SATELLITE_SUPPORT) := $(libpdl.a-objs-y)
libpdl.so-objs-$(CONFIG_PDL_BMC_SUPPORT) := $(libpdl.a-objs-y)
libpdl.so-objs-$(CONFIG_PDL_MCU_SUPPORT) := $(libpdl.a-objs-y)

# 私有编译标志（每个模块独立定义）
CFLAGS_PRIV := -I$(srctree)/core/pdl/include
CFLAGS_PRIV += -I$(srctree)/core/pcl/include
CFLAGS_PRIV += -I$(srctree)/core/hal/include
CFLAGS_PRIV += -I$(srctree)/core/osal/include
CFLAGS_PRIV += -I$(srctree)/include

# 私有链接标志和库（每个模块独立定义）
LDFLAGS_PRIV :=
LDLIBS_PRIV := -L$(objtree)/lib -lpcl -lhal -losal

# 包含统一构建框架
include $(scriptdir)/build/Makefile.build
