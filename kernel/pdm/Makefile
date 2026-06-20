# SPDX-License-Identifier: GPL-2.0

# Kbuild fragment for pdm.ko.
ifeq ($(CONFIG_PDM),y)
obj-m += pdm.o
pdm-y := pdm/src/pdm.o
pdm-y += pdm/src/base/pdm_ctl_chrdev.o
pdm-y += lpf/peripheral/lpf_peripheral.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_service.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_chrdev.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/peripheral/mcu/lpf_mcu_proc.o
pdm-$(CONFIG_LPF_MCU_SERVICE) += lpf/transport/mcu/lpf_mcu_transport.o
pdm-$(CONFIG_LPF_MCU_TRANSPORT_CAN) += lpf/transport/mcu/lpf_mcu_transport_can.o
pdm-$(CONFIG_LPF_MCU_TRANSPORT_UART) += lpf/transport/mcu/lpf_mcu_transport_uart.o
pdm-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_service.o
pdm-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_chrdev.o
pdm-$(CONFIG_LPF_LED_SERVICE) += lpf/peripheral/led/lpf_led_proc.o
endif
