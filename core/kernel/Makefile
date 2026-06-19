ESMW_ROOT := $(abspath $(src)/../..)
ESMW_AUTOCONF := $(ESMW_ROOT)/include/config/auto.conf
ESMW_AUTOCONF_H := $(ESMW_ROOT)/include/generated/gen_autoconf.h

-include $(ESMW_AUTOCONF)

include $(src)/osal/Makefile
include $(src)/pconfig/Makefile
include $(src)/hal/Makefile
include $(src)/pdm/Makefile

ccflags-y += -I$(src)/include
ccflags-y += -I$(src)/include/osal
ccflags-y += -I$(src)/include/hal
ccflags-y += -I$(src)/include/hal/config
ccflags-y += -I$(src)/include/pconfig
ccflags-y += -I$(src)/include/pdm
ccflags-y += -I$(src)/pdm/src/protocol
ccflags-y += -I$(src)/../uapi

ifneq ($(wildcard $(ESMW_AUTOCONF_H)),)
ccflags-y += -include $(ESMW_AUTOCONF_H)
endif

ccflags-$(CONFIG_PDM_MCU_SUPPORT) += -DCONFIG_PDM_MCU_SUPPORT=1
ccflags-$(CONFIG_PDM_PROTOCOL_MCU) += -DCONFIG_PDM_PROTOCOL_MCU=1
ccflags-$(CONFIG_HAL_CAN) += -DCONFIG_HAL_CAN=1
ccflags-$(CONFIG_HAL_UART) += -DCONFIG_HAL_UART=1
