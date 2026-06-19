# SPDX-License-Identifier: GPL-2.0

# Kbuild fragment for hal.ko.
ifeq ($(CONFIG_HAL),y)
obj-m += hal.o
hal-y := hal/src/hal.o
hal-y += hal/src/hal_builtin_start.o
hal-$(CONFIG_HAL_CAN) += hal/src/hal_can.o
hal-$(CONFIG_HAL_UART) += hal/src/hal_serial.o
hal-$(CONFIG_HAL_GPIO) += hal/src/hal_gpio.o
hal-$(CONFIG_HAL_PWM) += hal/src/hal_pwm.o
hal-$(CONFIG_HAL_I2C) += hal/src/hal_i2c.o
hal-$(CONFIG_HAL_SPI) += hal/src/hal_spi.o
hal-y += hal/src/hal_builtin_end.o
endif
