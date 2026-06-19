// SPDX-License-Identifier: GPL-2.0

#include "lpf/lpf_compat_can.h"
#include "lpf/lpf_compat_gpio.h"
#include "lpf/lpf_compat_i2c.h"
#include "lpf/lpf_compat_pwm.h"
#include "lpf/lpf_compat_serial.h"
#include "lpf/lpf_compat_spi.h"
#include "lpf/lpf_soc_adapter.h"

const lpf_soc_adapter_t g_lpf_soc_generic_linux_adapter = {
	.name = "generic-linux",
	.gpio = {
		.request = lpf_compat_gpio_request,
		.free = lpf_compat_gpio_free,
		.set_direction = lpf_compat_gpio_set_direction,
		.set_level = lpf_compat_gpio_set_level,
		.get_level = lpf_compat_gpio_get_level,
		.request_interrupt = lpf_compat_gpio_request_interrupt,
		.free_interrupt = lpf_compat_gpio_free_interrupt,
		.enable_interrupt = lpf_compat_gpio_enable_interrupt,
		.disable_interrupt = lpf_compat_gpio_disable_interrupt,
	},
	.pwm = {
		.get = lpf_compat_pwm_get,
		.put = lpf_compat_pwm_put,
		.apply = lpf_compat_pwm_apply,
		.get_state = lpf_compat_pwm_get_state,
		.enable = lpf_compat_pwm_enable,
		.disable = lpf_compat_pwm_disable,
	},
	.i2c = {
		.open = lpf_compat_i2c_open,
		.close = lpf_compat_i2c_close,
		.transfer = lpf_compat_i2c_transfer,
	},
	.spi = {
		.open = lpf_compat_spi_open,
		.close = lpf_compat_spi_close,
		.transfer = lpf_compat_spi_transfer,
		.set_config = lpf_compat_spi_set_config,
	},
	.can = {
		.init = lpf_compat_can_init,
		.deinit = lpf_compat_can_deinit,
		.send = lpf_compat_can_send,
		.recv = lpf_compat_can_recv,
		.set_filter = lpf_compat_can_set_filter,
	},
	.serial = {
		.open = lpf_compat_serial_open,
		.close = lpf_compat_serial_close,
		.write = lpf_compat_serial_write,
		.read = lpf_compat_serial_read,
		.flush = lpf_compat_serial_flush,
		.set_config = lpf_compat_serial_set_config,
	},
};
