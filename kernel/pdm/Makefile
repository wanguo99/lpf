# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_PDM) += pdm.o

# Core module entry
pdm-y := pdm/core/pdm_core.o

# Core bus and device model
pdm-y += pdm/core/bus/pdm_bus.o
pdm-y += pdm/core/device/pdm_device.o
pdm-y += pdm/core/device/pdm_of_bus.o

# Core userspace device nodes
pdm-y += pdm/core/chardev/pdm_ctl.o
pdm-y += pdm/core/chardev/pdm_client.o

# Core diagnostics interfaces
pdm-y += pdm/core/diag/pdm_debugfs.o
pdm-y += pdm/core/diag/pdm_proc.o
pdm-y += pdm/core/diag/pdm_sysfs.o

# Core driver/backend registries
pdm-y += pdm/core/driver/pdm_driver_registry.o
pdm-y += pdm/core/driver/pdm_backend_registry.o

# Automatic PDM bus driver entries must be linked between these markers.
pdm-y += pdm/core/driver/pdm_driver_registry_start.o
pdm-y += pdm/peripheral/mcu/pdm_mcu_driver.o

# Automatic PDM backend entries must be linked between these markers.
pdm-y += pdm/core/driver/pdm_backend_registry_start.o
pdm-y += pdm/peripheral/mcu/pdm_mcu_uart.o
pdm-$(CONFIG_PDM_MCU_UART_SERDEV) += pdm/peripheral/mcu/pdm_mcu_uart_serdev.o
pdm-$(CONFIG_PDM_MCU_CAN) += pdm/peripheral/mcu/pdm_mcu_can.o
pdm-$(CONFIG_PDM_MCU_I2C) += pdm/peripheral/mcu/pdm_mcu_i2c.o
pdm-$(CONFIG_PDM_MCU_SPI) += pdm/peripheral/mcu/pdm_mcu_spi.o
pdm-y += pdm/peripheral/led/pdm_led_driver.o
pdm-$(CONFIG_PDM_LED_GPIO) += pdm/peripheral/led/pdm_led_gpio.o
pdm-$(CONFIG_PDM_LED_PWM) += pdm/peripheral/led/pdm_led_pwm.o
pdm-y += pdm/core/driver/pdm_backend_registry_stop.o
pdm-y += pdm/core/driver/pdm_driver_registry_stop.o
pdm-$(CONFIG_PDM_MOCK_DEVICES) += pdm/mock/pdm_mock_devices.o
