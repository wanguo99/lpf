LPF_ROOT := $(abspath $(src)/..)
LPF_AUTOCONF := $(LPF_ROOT)/include/config/auto.conf
LPF_AUTOCONF_H := $(LPF_ROOT)/include/generated/gen_autoconf.h

-include $(LPF_AUTOCONF)

include $(src)/osal/Makefile
include $(src)/lpf/Makefile
include $(src)/hal/Makefile

ccflags-y += -I$(src)/include
ccflags-y += -I$(LPF_ROOT)/include
ccflags-y += -I$(src)/include/lpf
ccflags-y += -I$(src)/include/osal
ccflags-y += -I$(src)/include/hal
ccflags-y += -I$(src)/include/hal/config
ccflags-y += -I$(src)/hal/src
ccflags-y += -I$(src)/include/pconfig
ccflags-y += -I$(src)/lpf/peripheral/mcu
ccflags-y += -I$(src)/lpf/peripheral/led
ccflags-y += -I$(src)/lpf/transport/mcu
ccflags-y += -I$(src)/../uapi

ifneq ($(wildcard $(LPF_AUTOCONF_H)),)
ccflags-y += -include $(LPF_AUTOCONF_H)
endif
