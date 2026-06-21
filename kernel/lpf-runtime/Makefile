# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_LPF_CONFIGS) += lpf_configs.o
obj-$(CONFIG_LPF_HW_MOCK_SELFTEST) += lpf_hw_mock_selftest.o
obj-$(CONFIG_LPF_DUMMY_SERVICE_SELFTEST) += lpf_dummy_service_selftest.o

include $(src)/lpf-runtime/config/Makefile
include $(src)/lpf-runtime/hw/Makefile
include $(src)/lpf-runtime/hw/selftest/Makefile
include $(src)/lpf-runtime/runtime/Makefile
include $(src)/lpf-runtime/peripheral/mcu/Makefile
include $(src)/lpf-runtime/peripheral/led/Makefile
include $(src)/lpf-runtime/config/selftest/Makefile
lpf_core-$(CONFIG_LPF_RUNTIME) += lpf-runtime/runtime/lpf_runtime_entry_end.o
include $(src)/lpf-runtime/peripheral/selftest/Makefile
