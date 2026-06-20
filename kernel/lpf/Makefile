# SPDX-License-Identifier: GPL-2.0

ifeq ($(CONFIG_LPF_CORE),y)
obj-m += lpf_core.o
lpf_core-y := lpf/core/lpf_core.o
lpf_core-y += lpf/core/lpf_chrdev.o
lpf_core-y += lpf/core/lpf_ctl.o
lpf_core-y += lpf/core/lpf_debugfs.o
lpf_core-y += lpf/core/lpf_proc.o
lpf_core-y += lpf/core/lpf_sysfs.o
lpf_core-$(CONFIG_LPF_PROTOCOL) += lpf/protocol/lpf_protocol.o
lpf_core-$(CONFIG_LPF_PROTOCOL) += lpf/protocol/lpf_protocol_common.o
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

ifeq ($(CONFIG_LPF_PERIPHERAL_RUNTIME),y)
obj-m += lpf_peripheral_runtime.o
lpf_peripheral_runtime-y := lpf/peripheral/lpf_peripheral_module.o
lpf_peripheral_runtime-y += lpf/config/src/lpf_config_api.o
lpf_peripheral_runtime-y += lpf/config/src/lpf_config_backend.o
lpf_peripheral_runtime-y += lpf/config/src/lpf_config_dt_backend.o
lpf_peripheral_runtime-y += lpf/config/src/lpf_config_dt_parser.o
lpf_peripheral_runtime-y += lpf/config/src/lpf_config_static_backend.o
lpf_peripheral_runtime-y += lpf/config/src/lpf_config_normalizer.o
lpf_peripheral_runtime-y += lpf/config/src/lpf_config_validator.o
lpf_peripheral_runtime-y += lpf/config/configs/lpf_config_configs.o
lpf_peripheral_runtime-y += lpf/config/configs/kernel/x86_modules/1.0.0/lpf_config_kernel_x86_modules.o
lpf_peripheral_runtime-y += lpf/config/configs/kernel/x86_mock_modules/1.0.0/lpf_config_kernel_x86_mock_modules.o
lpf_peripheral_runtime-y += lpf/hw/lpf_hw.o
lpf_peripheral_runtime-y += lpf/hw/lpf_hw_builtin_start.o
lpf_peripheral_runtime-$(CONFIG_LPF_HW_CAN) += lpf/hw/lpf_hw_transport_can.o
lpf_peripheral_runtime-$(CONFIG_LPF_HW_UART) += lpf/hw/lpf_hw_transport_uart.o
lpf_peripheral_runtime-$(CONFIG_LPF_HW_GPIO) += lpf/hw/lpf_hw_gpio.o
lpf_peripheral_runtime-$(CONFIG_LPF_HW_PWM) += lpf/hw/lpf_hw_pwm.o
lpf_peripheral_runtime-$(CONFIG_LPF_HW_I2C) += lpf/hw/lpf_hw_bus_i2c.o
lpf_peripheral_runtime-$(CONFIG_LPF_HW_SPI) += lpf/hw/lpf_hw_bus_spi.o
lpf_peripheral_runtime-y += lpf/hw/lpf_hw_builtin_end.o
lpf_peripheral_runtime-y += lpf/peripheral/lpf_peripheral.o
lpf_peripheral_runtime-y += lpf/peripheral/lpf_peripheral_config.o
lpf_peripheral_runtime-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_service.o
lpf_peripheral_runtime-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_chrdev.o
lpf_peripheral_runtime-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_proc.o
lpf_peripheral_runtime-$(CONFIG_LPF_MCU_SERVICE) += lpf/transport/mcu/lpf_mcu_transport.o
lpf_peripheral_runtime-$(CONFIG_LPF_MCU_TRANSPORT_CAN) += lpf/transport/mcu/lpf_mcu_transport_can.o
lpf_peripheral_runtime-$(CONFIG_LPF_MCU_TRANSPORT_UART) += lpf/transport/mcu/lpf_mcu_transport_uart.o
lpf_peripheral_runtime-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_service.o
lpf_peripheral_runtime-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_chrdev.o
lpf_peripheral_runtime-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_proc.o
endif

ifeq ($(CONFIG_LPF_HW_MOCK_SELFTEST),y)
obj-m += lpf_hw_mock_selftest.o
lpf_hw_mock_selftest-y := lpf/hw/selftest/lpf_hw_mock_selftest.o
endif

ifeq ($(CONFIG_LPF_DUMMY_SERVICE_SELFTEST),y)
obj-m += lpf_dummy_service_selftest.o
lpf_dummy_service_selftest-y := lpf/peripheral/selftest/lpf_dummy_service_selftest.o
endif
endif
