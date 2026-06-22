# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_PDM_CORE) += pdm_core.o

# Bus implementation
pdm_core-y := pdm/bus/pdm_bus.o
pdm_core-y += pdm/bus/pdm_device.o
pdm_core-y += pdm/bus/pdm_bus_controller.o

# Core modules
pdm_core-y += pdm/core/pdm_core.o
pdm_core-y += pdm/core/pdm_debugfs.o
pdm_core-y += pdm/core/pdm_proc.o
pdm_core-y += pdm/core/pdm_sysfs.o
