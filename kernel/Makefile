PDM_ROOT := $(abspath $(src)/..)
PDM_AUTOCONF := $(PDM_ROOT)/include/config/auto.conf
PDM_AUTOCONF_H := $(PDM_ROOT)/include/generated/gen_autoconf.h

-include $(PDM_AUTOCONF)

include $(src)/osal/Makefile
include $(src)/pdm/Makefile

ccflags-y += -I$(src)/include
ccflags-y += -I$(PDM_ROOT)/include
ccflags-y += -I$(src)/include/osal
ccflags-y += -I$(src)/../uapi

ifneq ($(wildcard $(PDM_AUTOCONF_H)),)
ccflags-y += -include $(PDM_AUTOCONF_H)
endif
