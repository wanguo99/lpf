# SPDX-License-Identifier: GPL-2.0

# Kbuild fragment for pdm.ko.
ifeq ($(CONFIG_PDM),y)
obj-m += pdm.o
pdm-y := pdm/src/pdm.o
pdm-y += pdm/src/base/pdm_bus.o
pdm-y += pdm/src/base/pdm_chrdev.o
pdm-y += pdm/src/base/pdm_driver.o
pdm-y += pdm/src/base/pdm_driver_start.o
pdm-y += pdm/src/base/pdm_proc.o
pdm-y += pdm/src/base/pdm_status.o
pdm-$(CONFIG_PDM_PROTOCOL) += pdm/src/protocol/pdm_protocol.o
pdm-$(CONFIG_PDM_PROTOCOL) += pdm/src/protocol/pdm_protocol_common.o
pdm-$(CONFIG_PDM_MCU_SUPPORT) += pdm/src/mcu/pdm_mcu.o
pdm-$(CONFIG_PDM_MCU_SUPPORT) += pdm/src/mcu/pdm_mcu_chrdev.o
pdm-$(CONFIG_PDM_MCU_SUPPORT) += pdm/src/mcu/pdm_mcu_can.o
pdm-$(CONFIG_PDM_MCU_SUPPORT) += pdm/src/mcu/pdm_mcu_serial.o
pdm-$(CONFIG_PDM_LED_SUPPORT) += pdm/src/led/pdm_led.o
pdm-$(CONFIG_PDM_LED_SUPPORT) += pdm/src/led/pdm_led_chrdev.o
pdm-y += pdm/src/base/pdm_driver_end.o
endif
