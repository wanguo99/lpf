// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/hw/lpf_hw_can.h"
#include "lpf/hw/lpf_hw_gpio.h"
#include "lpf/hw/lpf_hw_i2c.h"
#include "lpf/hw/lpf_hw_pwm.h"
#include "lpf/hw/lpf_hw_uart.h"
#include "lpf/hw/lpf_hw_spi.h"

#define LPF_HW_MOCK_SELFTEST_GPIO 7U
#define LPF_HW_MOCK_SELFTEST_HIGH_GPIO 300U
#define LPF_HW_MOCK_SELFTEST_PWM_HANDLES 40U
#define LPF_HW_MOCK_SELFTEST_BUS_HANDLES 20U

typedef struct {
	uint32_t count;
	uint32_t gpio_num;
	lpf_gpio_level_t level;
} lpf_hw_mock_gpio_irq_state_t;

typedef struct {
	lpf_hw_pwm_handle_t pwm_handles[LPF_HW_MOCK_SELFTEST_PWM_HANDLES];
	lpf_hw_bus_i2c_handle_t i2c_handles[LPF_HW_MOCK_SELFTEST_BUS_HANDLES];
	lpf_hw_bus_spi_handle_t spi_handles[LPF_HW_MOCK_SELFTEST_BUS_HANDLES];
	lpf_hw_transport_can_handle_t can_handles[LPF_HW_MOCK_SELFTEST_BUS_HANDLES];
	lpf_hw_transport_uart_handle_t serial_handles[LPF_HW_MOCK_SELFTEST_BUS_HANDLES];
} lpf_hw_mock_dynamic_resource_handles_t;

static int lpf_hw_mock_status_to_errno(int32_t status)
{
	if (status == OSAL_SUCCESS)
		return 0;
	if (status > 0 && status < 4096)
		return -status;
	return -EINVAL;
}

static int32_t lpf_hw_mock_expect_success(const char *name, int32_t status)
{
	if (status == OSAL_SUCCESS)
		return OSAL_SUCCESS;

	pr_err("LPF:HW_SELFTEST: %s failed: %d\n", name, status);
	return status;
}

static void lpf_hw_mock_gpio_irq_callback(uint32_t gpio_num,
				       lpf_gpio_level_t level,
				       void *user_data)
{
	lpf_hw_mock_gpio_irq_state_t *state = user_data;

	if (!state)
		return;

	state->count++;
	state->gpio_num = gpio_num;
	state->level = level;
}

static int32_t lpf_hw_mock_selftest_gpio(void)
{
	lpf_hw_mock_gpio_irq_state_t irq_state = { 0 };
	lpf_gpio_config_t config = {
		.direction = LPF_GPIO_DIR_OUTPUT,
		.initial_level = LPF_GPIO_LEVEL_LOW,
		.edge = LPF_GPIO_EDGE_BOTH,
		.callback = lpf_hw_mock_gpio_irq_callback,
		.user_data = &irq_state,
	};
	lpf_gpio_direction_t direction;
	lpf_gpio_level_t level;
	int32_t ret;

	ret = lpf_hw_gpio_init(LPF_HW_MOCK_SELFTEST_GPIO, &config);
	if (ret != OSAL_SUCCESS)
		return lpf_hw_mock_expect_success("lpf_hw_gpio_init", ret);

	ret = lpf_hw_gpio_get_direction(LPF_HW_MOCK_SELFTEST_GPIO, &direction);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (direction != LPF_GPIO_DIR_OUTPUT) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_deinit;
	}

	ret = lpf_hw_gpio_get_level(LPF_HW_MOCK_SELFTEST_GPIO, &level);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (level != LPF_GPIO_LEVEL_LOW) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_deinit;
	}

	ret = lpf_hw_gpio_set_level(LPF_HW_MOCK_SELFTEST_GPIO, LPF_GPIO_LEVEL_HIGH);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (irq_state.count != 1U ||
	    irq_state.gpio_num != LPF_HW_MOCK_SELFTEST_GPIO ||
	    irq_state.level != LPF_GPIO_LEVEL_HIGH) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_deinit;
	}

	ret = lpf_hw_gpio_disable_interrupt(LPF_HW_MOCK_SELFTEST_GPIO);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	ret = lpf_hw_gpio_set_level(LPF_HW_MOCK_SELFTEST_GPIO, LPF_GPIO_LEVEL_LOW);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (irq_state.count != 1U) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_deinit;
	}

	ret = lpf_hw_gpio_enable_interrupt(LPF_HW_MOCK_SELFTEST_GPIO);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	ret = lpf_hw_gpio_set_level(LPF_HW_MOCK_SELFTEST_GPIO, LPF_GPIO_LEVEL_HIGH);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (irq_state.count != 2U || irq_state.level != LPF_GPIO_LEVEL_HIGH)
		ret = OSAL_ERR_INVALID_STATE;

out_deinit:
	(void)lpf_hw_gpio_deinit(LPF_HW_MOCK_SELFTEST_GPIO);
	return lpf_hw_mock_expect_success("gpio path", ret);
}

static int32_t lpf_hw_mock_selftest_pwm(void)
{
	lpf_pwm_config_t config = {
		.consumer = "lpf-mock-pwm0",
		.period_ns = 1000000U,
		.duty_ns = 250000U,
		.enabled = false,
		.polarity_inversed = false,
	};
	lpf_pwm_state_t state;
	lpf_hw_pwm_handle_t handle = NULL;
	int32_t ret;

	ret = lpf_hw_pwm_init(&config, &handle);
	if (ret != OSAL_SUCCESS)
		return lpf_hw_mock_expect_success("lpf_hw_pwm_init", ret);

	ret = lpf_hw_pwm_get_state(handle, &state);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (state.period_ns != config.period_ns ||
	    state.duty_ns != config.duty_ns || state.enabled) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_deinit;
	}

	state.duty_ns = 500000U;
	state.enabled = true;
	ret = lpf_hw_pwm_apply(handle, &state);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;

	ret = lpf_hw_pwm_get_state(handle, &state);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (state.duty_ns != 500000U || !state.enabled) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_deinit;
	}

	ret = lpf_hw_pwm_disable(handle);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	ret = lpf_hw_pwm_get_state(handle, &state);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (state.enabled) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_deinit;
	}

	ret = lpf_hw_pwm_enable(handle);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	ret = lpf_hw_pwm_get_state(handle, &state);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (!state.enabled)
		ret = OSAL_ERR_INVALID_STATE;

out_deinit:
	(void)lpf_hw_pwm_deinit(handle);
	return lpf_hw_mock_expect_success("pwm path", ret);
}

static int32_t lpf_hw_mock_selftest_can(void)
{
	lpf_can_config_t config = {
		.interface = "mock-can0",
		.baudrate = 500000U,
		.rx_timeout = 10U,
		.tx_timeout = 10U,
	};
	lpf_can_frame_t tx = {
		.can_id = 0x123U,
		.dlc = 3U,
		.data = { 0x11U, 0x22U, 0x33U },
	};
	lpf_can_frame_t rx;
	lpf_hw_transport_can_handle_t handle = NULL;
	int32_t ret;

	ret = lpf_hw_transport_can_init(&config, &handle);
	if (ret != OSAL_SUCCESS)
		return lpf_hw_mock_expect_success("lpf_hw_transport_can_init", ret);

	ret = lpf_hw_transport_can_set_filter(handle, tx.can_id, 0x7FFU);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	ret = lpf_hw_transport_can_send(handle, &tx);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;

	osal_memset(&rx, 0, sizeof(rx));
	ret = lpf_hw_transport_can_recv(handle, &rx, 10);
	if (ret != OSAL_SUCCESS)
		goto out_deinit;
	if (rx.can_id != tx.can_id || rx.dlc != tx.dlc ||
	    rx.data[0] != tx.data[0] || rx.data[1] != tx.data[1] ||
	    rx.data[2] != tx.data[2])
		ret = OSAL_ERR_INVALID_STATE;

out_deinit:
	(void)lpf_hw_transport_can_deinit(handle);
	return lpf_hw_mock_expect_success("can path", ret);
}

static int32_t lpf_hw_mock_selftest_serial(void)
{
	lpf_serial_config_t config = {
		.baud_rate = 115200U,
		.data_bits = 8U,
		.stop_bits = 1U,
		.parity = LPF_SERIAL_PARITY_NONE,
		.flow_control = LPF_SERIAL_FLOW_NONE,
	};
	uint8_t tx[] = { 0x41U, 0x42U, 0x43U };
	uint8_t rx[sizeof(tx)];
	lpf_hw_transport_uart_handle_t handle = NULL;
	int32_t ret;

	ret = lpf_hw_transport_uart_open("mock-tty0", &config, &handle);
	if (ret != OSAL_SUCCESS)
		return lpf_hw_mock_expect_success("lpf_hw_transport_uart_open", ret);

	ret = lpf_hw_transport_uart_write(handle, tx, sizeof(tx), 10);
	if (ret != (int32_t)sizeof(tx)) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_close;
	}

	osal_memset(rx, 0, sizeof(rx));
	ret = lpf_hw_transport_uart_read(handle, rx, sizeof(rx), 10);
	if (ret != (int32_t)sizeof(rx)) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_close;
	}
	if (osal_memcmp(rx, tx, sizeof(tx)) != 0) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_close;
	}

	config.baud_rate = 57600U;
	ret = lpf_hw_transport_uart_set_config(handle, &config);
	if (ret != OSAL_SUCCESS)
		goto out_close;
	ret = lpf_hw_transport_uart_flush(handle);
	if (ret != OSAL_SUCCESS)
		goto out_close;
	ret = lpf_hw_transport_uart_read(handle, rx, sizeof(rx), 0);
	if (ret != 0)
		ret = OSAL_ERR_INVALID_STATE;

out_close:
	(void)lpf_hw_transport_uart_close(handle);
	return lpf_hw_mock_expect_success("serial path", ret);
}

static int32_t lpf_hw_mock_selftest_i2c(void)
{
	lpf_i2c_config_t config = {
		.device = "i2c-mock0",
		.timeout = 10U,
	};
	uint8_t write_buf[] = { 0x10U, 0x20U };
	uint8_t read_buf[4];
	lpf_hw_bus_i2c_handle_t handle = NULL;
	int32_t ret;

	ret = lpf_hw_bus_i2c_open(&config, &handle);
	if (ret != OSAL_SUCCESS)
		return lpf_hw_mock_expect_success("lpf_hw_bus_i2c_open", ret);

	ret = lpf_hw_bus_i2c_write(handle, 0x50U, write_buf, sizeof(write_buf));
	if (ret != OSAL_SUCCESS)
		goto out_close;

	osal_memset(read_buf, 0, sizeof(read_buf));
	ret = lpf_hw_bus_i2c_read(handle, 0x50U, read_buf, sizeof(read_buf));
	if (ret != OSAL_SUCCESS)
		goto out_close;
	if (read_buf[0] != 0U || read_buf[1] != 1U ||
	    read_buf[2] != 2U || read_buf[3] != 3U) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_close;
	}

	ret = lpf_hw_bus_i2c_write_reg(handle, 0x50U, 0x01U, write_buf,
				sizeof(write_buf));
	if (ret != OSAL_SUCCESS)
		goto out_close;

	osal_memset(read_buf, 0, sizeof(read_buf));
	ret = lpf_hw_bus_i2c_read_reg(handle, 0x50U, 0x01U, read_buf,
			       sizeof(read_buf));
	if (ret != OSAL_SUCCESS)
		goto out_close;
	if (read_buf[3] != 3U)
		ret = OSAL_ERR_INVALID_STATE;

out_close:
	lpf_hw_bus_i2c_close(handle);
	return lpf_hw_mock_expect_success("i2c path", ret);
}

static int32_t lpf_hw_mock_selftest_spi(void)
{
	lpf_spi_config_t config = {
		.device = "mock-spi0.0",
		.mode = LPF_SPI_MODE_0,
		.bits_per_word = 8U,
		.max_speed_hz = 1000000U,
		.timeout = 10U,
	};
	uint8_t tx[] = { 0xA5U, 0x5AU };
	uint8_t rx[sizeof(tx)];
	lpf_hw_bus_spi_handle_t handle = NULL;
	int32_t ret;

	ret = lpf_hw_bus_spi_open(&config, &handle);
	if (ret != OSAL_SUCCESS)
		return lpf_hw_mock_expect_success("lpf_hw_bus_spi_open", ret);

	osal_memset(rx, 0, sizeof(rx));
	ret = lpf_hw_bus_spi_transfer(handle, tx, rx, sizeof(tx));
	if (ret != OSAL_SUCCESS)
		goto out_close;
	if (osal_memcmp(rx, tx, sizeof(tx)) != 0) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_close;
	}

	osal_memset(rx, 0xFF, sizeof(rx));
	ret = lpf_hw_bus_spi_read(handle, rx, sizeof(rx));
	if (ret != OSAL_SUCCESS)
		goto out_close;
	if (rx[0] != 0U || rx[1] != 0U) {
		ret = OSAL_ERR_INVALID_STATE;
		goto out_close;
	}

	config.max_speed_hz = 2000000U;
	ret = lpf_hw_bus_spi_set_config(handle, &config);

out_close:
	lpf_hw_bus_spi_close(handle);
	return lpf_hw_mock_expect_success("spi path", ret);
}

static int32_t lpf_hw_mock_selftest_dynamic_resources(void)
{
	lpf_gpio_config_t gpio_config = {
		.direction = LPF_GPIO_DIR_OUTPUT,
		.initial_level = LPF_GPIO_LEVEL_LOW,
		.edge = LPF_GPIO_EDGE_NONE,
	};
	lpf_pwm_config_t pwm_config = {
		.consumer = "lpf-mock-pwm-scale",
		.period_ns = 1000000U,
		.duty_ns = 100000U,
	};
	lpf_i2c_config_t i2c_config = {
		.device = "i2c-mock-scale",
		.timeout = 10U,
	};
	lpf_spi_config_t spi_config = {
		.device = "mock-spi-scale",
		.mode = LPF_SPI_MODE_0,
		.bits_per_word = 8U,
		.max_speed_hz = 1000000U,
		.timeout = 10U,
	};
	lpf_can_config_t can_config = {
		.interface = "mock-can-scale",
		.baudrate = 500000U,
		.rx_timeout = 10U,
		.tx_timeout = 10U,
	};
	lpf_serial_config_t serial_config = {
		.baud_rate = 115200U,
		.data_bits = 8U,
		.stop_bits = 1U,
		.parity = LPF_SERIAL_PARITY_NONE,
		.flow_control = LPF_SERIAL_FLOW_NONE,
	};
	lpf_hw_mock_dynamic_resource_handles_t *handles;
	bool gpio_initialized = false;
	uint32_t i;
	int32_t ret = OSAL_SUCCESS;

	handles = osal_zalloc(sizeof(*handles));
	if (!handles)
		return lpf_hw_mock_expect_success("dynamic resource paths",
						  OSAL_ERR_NO_MEMORY);

	ret = lpf_hw_gpio_init(LPF_HW_MOCK_SELFTEST_HIGH_GPIO, &gpio_config);
	if (ret != OSAL_SUCCESS)
		goto out;
	gpio_initialized = true;

	for (i = 0; i < LPF_HW_MOCK_SELFTEST_PWM_HANDLES; i++) {
		ret = lpf_hw_pwm_init(&pwm_config, &handles->pwm_handles[i]);
		if (ret != OSAL_SUCCESS)
			goto out;
	}

	for (i = 0; i < LPF_HW_MOCK_SELFTEST_BUS_HANDLES; i++) {
		ret = lpf_hw_bus_i2c_open(&i2c_config,
					   &handles->i2c_handles[i]);
		if (ret != OSAL_SUCCESS)
			goto out;
		ret = lpf_hw_bus_spi_open(&spi_config,
					   &handles->spi_handles[i]);
		if (ret != OSAL_SUCCESS)
			goto out;
		ret = lpf_hw_transport_can_init(&can_config,
						 &handles->can_handles[i]);
		if (ret != OSAL_SUCCESS)
			goto out;
		ret = lpf_hw_transport_uart_open("mock-tty-scale",
						 &serial_config,
						 &handles->serial_handles[i]);
		if (ret != OSAL_SUCCESS)
			goto out;
	}

out:
	for (i = 0; i < LPF_HW_MOCK_SELFTEST_BUS_HANDLES; i++) {
		if (handles->serial_handles[i])
			(void)lpf_hw_transport_uart_close(
				handles->serial_handles[i]);
		if (handles->can_handles[i])
			(void)lpf_hw_transport_can_deinit(
				handles->can_handles[i]);
		if (handles->spi_handles[i])
			(void)lpf_hw_bus_spi_close(handles->spi_handles[i]);
		if (handles->i2c_handles[i])
			(void)lpf_hw_bus_i2c_close(handles->i2c_handles[i]);
	}
	for (i = 0; i < LPF_HW_MOCK_SELFTEST_PWM_HANDLES; i++) {
		if (handles->pwm_handles[i])
			(void)lpf_hw_pwm_deinit(handles->pwm_handles[i]);
	}
	if (gpio_initialized)
		(void)lpf_hw_gpio_deinit(LPF_HW_MOCK_SELFTEST_HIGH_GPIO);
	osal_free(handles);

	return lpf_hw_mock_expect_success("dynamic resource paths", ret);
}

static int32_t lpf_hw_mock_selftest_run(void)
{
	int32_t ret;

	ret = lpf_hw_mock_selftest_gpio();
	if (ret != OSAL_SUCCESS)
		return ret;
	ret = lpf_hw_mock_selftest_pwm();
	if (ret != OSAL_SUCCESS)
		return ret;
	ret = lpf_hw_mock_selftest_can();
	if (ret != OSAL_SUCCESS)
		return ret;
	ret = lpf_hw_mock_selftest_serial();
	if (ret != OSAL_SUCCESS)
		return ret;
	ret = lpf_hw_mock_selftest_i2c();
	if (ret != OSAL_SUCCESS)
		return ret;
	ret = lpf_hw_mock_selftest_spi();
	if (ret != OSAL_SUCCESS)
		return ret;
	return lpf_hw_mock_selftest_dynamic_resources();
}

static int __init lpf_hw_mock_selftest_init(void)
{
	int32_t ret;

	ret = lpf_hw_mock_selftest_run();
	if (ret != OSAL_SUCCESS)
		return lpf_hw_mock_status_to_errno(ret);

	pr_info("LPF:HW_SELFTEST: mock LPF HW path checks passed\n");
	return 0;
}

static void __exit lpf_hw_mock_selftest_exit(void)
{
	pr_info("LPF:HW_SELFTEST: unloaded\n");
}

module_init(lpf_hw_mock_selftest_init);
module_exit(lpf_hw_mock_selftest_exit);

MODULE_AUTHOR("LPF");
MODULE_DESCRIPTION("LPF HW mock backend self-test");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: osal lpf_core");
