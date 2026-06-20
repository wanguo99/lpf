# SPDX-License-Identifier: GPL-2.0

# Kbuild fragment for pdm.ko.
ifeq ($(CONFIG_PDM),y)
obj-m += pdm.o
pdm-y := pdm/src/pdm.o
pdm-y += pdm/src/base/pdm_ctl_chrdev.o
pdm-y += pdm/src/base/pdm_driver.o
pdm-y += pdm/src/base/pdm_proc.o
pdm-y += pdm/src/base/pdm_status.o
pdm-$(CONFIG_PDM_PROTOCOL) += pdm/src/protocol/pdm_protocol.o
pdm-$(CONFIG_PDM_PROTOCOL) += pdm/src/protocol/pdm_protocol_common.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_service.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_chrdev.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_proc.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_can.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_serial.o
pdm-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_service.o
pdm-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_chrdev.o
pdm-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_proc.o
endif
