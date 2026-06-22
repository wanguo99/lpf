# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_LPF_CORE) += lpf_core.o

include $(src)/lpf-core/core/Makefile
include $(src)/lpf-core/protocol/Makefile
include $(src)/lpf-core/compat/Makefile
include $(src)/lpf-core/soc/Makefile
include $(src)/lpf-core/hw/Makefile
include $(src)/lpf-core/hw/selftest/Makefile
include $(src)/lpf-core/runtime/Makefile
include $(src)/lpf-core/peripheral/mcu/Makefile
include $(src)/lpf-core/peripheral/led/Makefile
lpf_core-$(CONFIG_LPF_RUNTIME) += lpf-core/runtime/lpf_runtime_entry_end.o
include $(src)/lpf-core/peripheral/selftest/Makefile
