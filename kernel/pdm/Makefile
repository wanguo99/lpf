# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_PDM) += pdm.o

# Core module entry
pdm-y := pdm/pdm.o

# Core bus and device model
pdm-y += pdm/core/bus/pdm_bus.o
pdm-y += pdm/core/bus/pdm_device.o
pdm-y += pdm/core/bus/pdm_of_bus.o

# Core userspace device nodes
pdm-y += pdm/core/cdev/pdm_cdev.o

# Core diagnostics interfaces
pdm-y += pdm/core/diag/pdm_debugfs.o
pdm-y += pdm/core/diag/pdm_proc.o
pdm-y += pdm/core/diag/pdm_sysfs.o

# Core driver/backend registries
pdm-y += pdm/core/registry/pdm_driver_registry.o
pdm-y += pdm/core/registry/pdm_backend_registry.o

pdm-y += pdm/drivers/mcu/pdm_mcu_protocol.o

# Automatic PDM bus driver and backend entries
pdm-y += pdm/drivers/mcu/pdm_mcu.o
pdm-y += pdm/drivers/mcu/pdm_mcu_uart.o
pdm-$(CONFIG_PDM_MCU_UART_SERDEV) += pdm/drivers/mcu/pdm_mcu_uart_serdev.o
pdm-$(CONFIG_PDM_MCU_CAN) += pdm/drivers/mcu/pdm_mcu_can.o
pdm-$(CONFIG_PDM_MCU_I2C) += pdm/drivers/mcu/pdm_mcu_i2c.o
pdm-$(CONFIG_PDM_MCU_SPI) += pdm/drivers/mcu/pdm_mcu_spi.o
pdm-y += pdm/drivers/led/pdm_led.o
pdm-$(CONFIG_PDM_LED_GPIO) += pdm/drivers/led/pdm_led_gpio.o
pdm-$(CONFIG_PDM_LED_PWM) += pdm/drivers/led/pdm_led_pwm.o
pdm-$(CONFIG_PDM_MOCK_DEVICES) += pdm/drivers/mock/pdm_mock_devices.o
