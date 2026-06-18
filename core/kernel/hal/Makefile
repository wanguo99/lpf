# SPDX-License-Identifier: GPL-2.0

# Object list exported for kernel modules that consume HAL internally.
ESMW_HAL_OBJS :=

ESMW_HAL_OBJS-$(CONFIG_HAL_CAN) += hal/src/kernel/hal_can_kernel.o
ESMW_HAL_OBJS-$(CONFIG_HAL_UART) += hal/src/kernel/hal_serial_kernel.o

ESMW_HAL_OBJS += $(ESMW_HAL_OBJS-y)
