# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_LPF_CONFIGS) += lpf_configs.o

lpf_configs-y += lpf-configs/core/lpf_config_api.o
lpf_configs-y += lpf-configs/core/lpf_config_backend.o
lpf_configs-y += lpf-configs/core/lpf_config_normalizer.o
lpf_configs-y += lpf-configs/core/lpf_config_validator.o
lpf_configs-y += lpf-configs/parser/static/lpf_config_static_backend.o
lpf_configs-y += lpf-configs/parser/static/lpf_config_static_start.o
lpf_configs-y += lpf-configs/parser/static/lpf_config_configs.o
lpf_configs-y += lpf-configs/parser/static/lpf_config_static_end.o
lpf_configs-$(CONFIG_OF) += lpf-configs/parser/dt/lpf_config_dt_backend.o
lpf_configs-$(CONFIG_OF) += lpf-configs/parser/dt/lpf_config_dt_parser.o
lpf_configs-$(CONFIG_LPF_CONFIG_OF_SELFTEST) += lpf-configs/selftest/lpf_config_of_selftest.o
lpf_configs-$(CONFIG_LPF_CONFIG_IMX6ULL_MODULES) += lpf-configs/products/imx6ull/modules/lpf_config_imx6ull_modules_v1.o
lpf_configs-$(CONFIG_LPF_CONFIG_UBUNTU_X86_MODULES) += lpf-configs/products/ubuntu/x86_modules/lpf_config_ubuntu_x86_modules_v1.o
lpf_configs-$(CONFIG_LPF_CONFIG_UBUNTU_X86_MOCK_MODULES) += lpf-configs/products/ubuntu/x86_mock_modules/lpf_config_ubuntu_x86_mock_modules_v1.o
