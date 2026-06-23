# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_PDM) += pdm.o

# Bus implementation
pdm-y := pdm/bus/pdm_bus.o
pdm-y += pdm/bus/pdm_device.o
pdm-y += pdm/bus/pdm_driver_registry.o
pdm-y += pdm/bus/pdm_bus_controller.o

# Core modules
pdm-y += pdm/core/pdm_core.o
pdm-y += pdm/core/pdm_debugfs.o
pdm-y += pdm/core/pdm_proc.o
pdm-y += pdm/core/pdm_sysfs.o
pdm-y += pdm/core/pdm_ctl.o
pdm-y += pdm/core/pdm_client.o
pdm-y += pdm/core/pdm_backend.o

# Automatic PDM bus driver entries must be linked between these markers.
pdm-y += pdm/bus/pdm_driver_registry_start.o
pdm-y += pdm/peripheral/mcu/pdm_mcu_driver.o

# Automatic PDM backend entries must be linked between these markers.
pdm-y += pdm/core/pdm_backend_start.o
pdm-y += pdm/peripheral/mcu/pdm_mcu_uart.o
pdm-y += pdm/peripheral/mcu/pdm_mcu_can.o
pdm-$(CONFIG_PDM_MCU_I2C) += pdm/peripheral/mcu/pdm_mcu_i2c.o
pdm-$(CONFIG_PDM_MCU_SPI) += pdm/peripheral/mcu/pdm_mcu_spi.o
pdm-y += pdm/peripheral/led/pdm_led_driver.o
pdm-y += pdm/peripheral/led/pdm_led_gpio.o
pdm-y += pdm/peripheral/led/pdm_led_pwm.o
pdm-y += pdm/core/pdm_backend_stop.o
pdm-y += pdm/bus/pdm_driver_registry_stop.o
pdm-$(CONFIG_PDM_MOCK_DEVICES) += pdm/mock/pdm_mock_devices.o
