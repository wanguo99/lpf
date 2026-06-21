# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_LPF_CONFIGS) += lpf_configs.o

lpf_configs-y += lpf-configs/lpf_config_static_start.o
lpf_configs-$(CONFIG_LPF_CONFIG_KERNEL_X86_MODULES) += lpf-configs/configs/kernel/x86_modules/lpf_config_kernel_x86_modules_v1.o
lpf_configs-$(CONFIG_LPF_CONFIG_KERNEL_X86_MOCK_MODULES) += lpf-configs/configs/kernel/x86_mock_modules/lpf_config_kernel_x86_mock_modules_v1.o
lpf_configs-y += lpf-configs/lpf_config_configs.o
lpf_configs-y += lpf-configs/lpf_config_static_end.o
