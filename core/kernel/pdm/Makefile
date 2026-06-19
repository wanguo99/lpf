# SPDX-License-Identifier: GPL-2.0

# Kbuild fragment for pdm.ko.
ifeq ($(CONFIG_PDM),y)
obj-m += pdm.o
pdm-y := pdm/src/pdm_main.o
pdm-$(CONFIG_PCONFIG) += $(ESMW_PCONFIG_OBJS)
pdm-$(CONFIG_PDM_PROTOCOL) += pdm/src/protocol/pdm_protocol.o
pdm-$(CONFIG_PDM_PROTOCOL) += pdm/src/protocol/pdm_protocol_common.o
pdm-$(CONFIG_PDM_PROTOCOL) += pdm/src/protocol/pdm_protocol_device.o
pdm-$(CONFIG_PDM_MCU_SUPPORT) += pdm/src/pdm_mcu/pdm_mcu.o
pdm-$(CONFIG_PDM_MCU_SUPPORT) += pdm/src/pdm_mcu/pdm_mcu_can.o
pdm-$(CONFIG_PDM_MCU_SUPPORT) += pdm/src/pdm_mcu/pdm_mcu_serial.o
pdm-$(CONFIG_PDM_MCU_SUPPORT) += $(ESMW_HAL_OBJS)
endif
