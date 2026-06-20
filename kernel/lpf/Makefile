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
lpf_peripheral_runtime-y += pconfig/src/pconfig_api.o
lpf_peripheral_runtime-y += pconfig/src/pconfig_backend.o
lpf_peripheral_runtime-y += pconfig/src/pconfig_dt_backend.o
lpf_peripheral_runtime-y += pconfig/src/pconfig_static_backend.o
lpf_peripheral_runtime-y += pconfig/src/pconfig_validator.o
lpf_peripheral_runtime-y += pconfig/configs/pconfig_configs.o
lpf_peripheral_runtime-y += pconfig/configs/kernel/x86_modules/1.0.0/pconfig_kernel_x86_modules.o
lpf_peripheral_runtime-y += hal/src/hal.o
lpf_peripheral_runtime-y += hal/src/hal_builtin_start.o
lpf_peripheral_runtime-$(CONFIG_HAL_CAN) += hal/src/hal_can.o
lpf_peripheral_runtime-$(CONFIG_HAL_UART) += hal/src/hal_serial.o
lpf_peripheral_runtime-$(CONFIG_HAL_GPIO) += hal/src/hal_gpio.o
lpf_peripheral_runtime-$(CONFIG_HAL_PWM) += hal/src/hal_pwm.o
lpf_peripheral_runtime-$(CONFIG_HAL_I2C) += hal/src/hal_i2c.o
lpf_peripheral_runtime-$(CONFIG_HAL_SPI) += hal/src/hal_spi.o
lpf_peripheral_runtime-y += hal/src/hal_builtin_end.o
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
endif
