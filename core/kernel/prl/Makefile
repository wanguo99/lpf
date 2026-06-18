# SPDX-License-Identifier: GPL-2.0

# Object list exported for kernel modules that consume PRL internally.
ESMW_PRL_OBJS :=

ifeq ($(CONFIG_PRL),y)
ESMW_PRL_OBJS += prl/src/prl_api.o
ESMW_PRL_OBJS += prl/src/prl_common.o
ESMW_PRL_OBJS += prl/src/prl_device.o
endif
