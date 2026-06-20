# SPDX-License-Identifier: GPL-2.0

ifeq ($(CONFIG_LPF_CORE),y)
obj-m += lpf_core.o
lpf_core-y := lpf/core/lpf_core.o
lpf_core-y += lpf/core/lpf_chrdev.o
lpf_core-y += lpf/core/lpf_debugfs.o
lpf_core-y += lpf/core/lpf_sysfs.o
lpf_core-y += lpf/compat/lpf_compat_gpio.o
lpf_core-y += lpf/compat/lpf_compat_pwm.o
lpf_core-y += lpf/compat/lpf_compat_i2c.o
lpf_core-y += lpf/compat/lpf_compat_spi.o
lpf_core-y += lpf/compat/lpf_compat_can.o
lpf_core-y += lpf/compat/lpf_compat_serial.o
lpf_core-y += lpf/soc/lpf_soc_adapter.o
lpf_core-$(CONFIG_LPF_SOC_ADAPTER_MOCK) += lpf/soc/mock/lpf_soc_mock.o
ifneq ($(CONFIG_LPF_SOC_ADAPTER_MOCK),y)
lpf_core-y += lpf/soc/generic_linux/lpf_soc_generic_linux.o
endif
endif
