// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "lpf/soc/lpf_soc_adapter.h"

#if defined(CONFIG_LPF_SOC_ADAPTER_MOCK)
extern const lpf_soc_adapter_t g_lpf_soc_mock_adapter;
#define LPF_SOC_DEFAULT_ADAPTER g_lpf_soc_mock_adapter
#else
extern const lpf_soc_adapter_t g_lpf_soc_generic_linux_adapter;
#define LPF_SOC_DEFAULT_ADAPTER g_lpf_soc_generic_linux_adapter
#endif

static osal_mutex_t g_lpf_soc_lock;
static bool g_lpf_soc_ready;
static const lpf_soc_adapter_t *g_lpf_soc_adapter;

static const lpf_soc_adapter_t *lpf_soc_get_adapter(void)
{
	const lpf_soc_adapter_t *adapter;

	if (!g_lpf_soc_ready)
		return NULL;

	osal_mutex_lock(&g_lpf_soc_lock);
	adapter = g_lpf_soc_adapter;
	osal_mutex_unlock(&g_lpf_soc_lock);
	return adapter;
}

int32_t lpf_soc_adapter_init(void)
{
	int32_t ret;

	if (g_lpf_soc_ready)
		return OSAL_SUCCESS;

	ret = osal_mutex_init(&g_lpf_soc_lock, NULL);
	if (ret != OSAL_SUCCESS)
		return ret;

	g_lpf_soc_ready = true;
	ret = lpf_soc_adapter_register(&LPF_SOC_DEFAULT_ADAPTER);
	if (ret != OSAL_SUCCESS) {
		g_lpf_soc_ready = false;
		osal_mutex_destroy(&g_lpf_soc_lock);
		return ret;
	}

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_soc_adapter_init);

void lpf_soc_adapter_deinit(void)
{
	if (!g_lpf_soc_ready)
		return;

	osal_mutex_lock(&g_lpf_soc_lock);
	g_lpf_soc_adapter = NULL;
	osal_mutex_unlock(&g_lpf_soc_lock);
	osal_mutex_destroy(&g_lpf_soc_lock);
	g_lpf_soc_ready = false;
}
EXPORT_SYMBOL_GPL(lpf_soc_adapter_deinit);

int32_t lpf_soc_adapter_register(const lpf_soc_adapter_t *adapter)
{
	if (!g_lpf_soc_ready || !adapter || !adapter->name)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&g_lpf_soc_lock);
	if (g_lpf_soc_adapter) {
		osal_mutex_unlock(&g_lpf_soc_lock);
		return OSAL_ERR_ALREADY_EXISTS;
	}

	g_lpf_soc_adapter = adapter;
	osal_mutex_unlock(&g_lpf_soc_lock);

	LOG_INFO("LPF", "registered SoC adapter %s", adapter->name);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_soc_adapter_register);

void lpf_soc_adapter_unregister(const lpf_soc_adapter_t *adapter)
{
	if (!g_lpf_soc_ready || !adapter)
		return;

	osal_mutex_lock(&g_lpf_soc_lock);
	if (g_lpf_soc_adapter == adapter)
		g_lpf_soc_adapter = NULL;
	osal_mutex_unlock(&g_lpf_soc_lock);
}
EXPORT_SYMBOL_GPL(lpf_soc_adapter_unregister);

const lpf_soc_adapter_t *lpf_soc_adapter_current(void)
{
	return lpf_soc_get_adapter();
}
EXPORT_SYMBOL_GPL(lpf_soc_adapter_current);

const char *lpf_soc_adapter_name(void)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	return adapter ? adapter->name : NULL;
}
EXPORT_SYMBOL_GPL(lpf_soc_adapter_name);

int32_t lpf_soc_gpio_request(uint32_t gpio_num, const char *label)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->gpio.request)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->gpio.request(gpio_num, label);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_request);

void lpf_soc_gpio_free(uint32_t gpio_num)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (adapter && adapter->gpio.free)
		adapter->gpio.free(gpio_num);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_free);

int32_t lpf_soc_gpio_set_direction(uint32_t gpio_num,
				   lpf_gpio_direction_t direction,
				   lpf_gpio_level_t initial_level)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->gpio.set_direction)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->gpio.set_direction(gpio_num, direction, initial_level);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_set_direction);

int32_t lpf_soc_gpio_set_level(uint32_t gpio_num, lpf_gpio_level_t level)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->gpio.set_level)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->gpio.set_level(gpio_num, level);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_set_level);

int32_t lpf_soc_gpio_get_level(uint32_t gpio_num, lpf_gpio_level_t *level)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->gpio.get_level)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->gpio.get_level(gpio_num, level);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_get_level);

int32_t lpf_soc_gpio_request_interrupt(uint32_t gpio_num,
				       lpf_gpio_edge_t edge,
				       lpf_gpio_irq_callback_t callback,
				       void *user_data, void **irq_handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->gpio.request_interrupt)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->gpio.request_interrupt(gpio_num, edge, callback,
					       user_data, irq_handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_request_interrupt);

void lpf_soc_gpio_free_interrupt(void *irq_handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (adapter && adapter->gpio.free_interrupt)
		adapter->gpio.free_interrupt(irq_handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_free_interrupt);

int32_t lpf_soc_gpio_enable_interrupt(void *irq_handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->gpio.enable_interrupt)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->gpio.enable_interrupt(irq_handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_enable_interrupt);

int32_t lpf_soc_gpio_disable_interrupt(void *irq_handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->gpio.disable_interrupt)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->gpio.disable_interrupt(irq_handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_gpio_disable_interrupt);

int32_t lpf_soc_pwm_get(const char *consumer, lpf_pwm_handle_t *handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->pwm.get)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->pwm.get(consumer, handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_pwm_get);

void lpf_soc_pwm_put(lpf_pwm_handle_t handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (adapter && adapter->pwm.put)
		adapter->pwm.put(handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_pwm_put);

int32_t lpf_soc_pwm_apply(lpf_pwm_handle_t handle,
			  const lpf_pwm_state_t *state)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->pwm.apply)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->pwm.apply(handle, state);
}
EXPORT_SYMBOL_GPL(lpf_soc_pwm_apply);

int32_t lpf_soc_pwm_get_state(lpf_pwm_handle_t handle,
			      lpf_pwm_state_t *state)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->pwm.get_state)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->pwm.get_state(handle, state);
}
EXPORT_SYMBOL_GPL(lpf_soc_pwm_get_state);

int32_t lpf_soc_pwm_enable(lpf_pwm_handle_t handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->pwm.enable)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->pwm.enable(handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_pwm_enable);

int32_t lpf_soc_pwm_disable(lpf_pwm_handle_t handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->pwm.disable)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->pwm.disable(handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_pwm_disable);

int32_t lpf_soc_i2c_open(const char *device, lpf_i2c_handle_t *handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->i2c.open)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->i2c.open(device, handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_i2c_open);

void lpf_soc_i2c_close(lpf_i2c_handle_t handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (adapter && adapter->i2c.close)
		adapter->i2c.close(handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_i2c_close);

int32_t lpf_soc_i2c_transfer(lpf_i2c_handle_t handle,
			     lpf_i2c_msg_t *msgs, uint32_t num)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->i2c.transfer)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->i2c.transfer(handle, msgs, num);
}
EXPORT_SYMBOL_GPL(lpf_soc_i2c_transfer);

int32_t lpf_soc_spi_open(const lpf_spi_config_t *config,
			 lpf_spi_handle_t *handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->spi.open)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->spi.open(config, handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_spi_open);

void lpf_soc_spi_close(lpf_spi_handle_t handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (adapter && adapter->spi.close)
		adapter->spi.close(handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_spi_close);

int32_t lpf_soc_spi_transfer(lpf_spi_handle_t handle,
			     const lpf_spi_transfer_t *transfers,
			     uint32_t num)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->spi.transfer)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->spi.transfer(handle, transfers, num);
}
EXPORT_SYMBOL_GPL(lpf_soc_spi_transfer);

int32_t lpf_soc_spi_set_config(lpf_spi_handle_t handle,
			       const lpf_spi_config_t *config)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->spi.set_config)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->spi.set_config(handle, config);
}
EXPORT_SYMBOL_GPL(lpf_soc_spi_set_config);

int32_t lpf_soc_can_init(const lpf_can_config_t *config,
			 lpf_can_handle_t *handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->can.init)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->can.init(config, handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_can_init);

int32_t lpf_soc_can_deinit(lpf_can_handle_t handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->can.deinit)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->can.deinit(handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_can_deinit);

int32_t lpf_soc_can_send(lpf_can_handle_t handle,
			 const lpf_can_frame_t *frame)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->can.send)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->can.send(handle, frame);
}
EXPORT_SYMBOL_GPL(lpf_soc_can_send);

int32_t lpf_soc_can_recv(lpf_can_handle_t handle, lpf_can_frame_t *frame,
			 int32_t timeout)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->can.recv)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->can.recv(handle, frame, timeout);
}
EXPORT_SYMBOL_GPL(lpf_soc_can_recv);

int32_t lpf_soc_can_set_filter(lpf_can_handle_t handle, uint32_t filter_id,
			       uint32_t filter_mask)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->can.set_filter)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->can.set_filter(handle, filter_id, filter_mask);
}
EXPORT_SYMBOL_GPL(lpf_soc_can_set_filter);

int32_t lpf_soc_serial_open(const char *device,
			    const lpf_serial_config_t *config,
			    lpf_serial_handle_t *handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->serial.open)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->serial.open(device, config, handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_serial_open);

int32_t lpf_soc_serial_close(lpf_serial_handle_t handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->serial.close)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->serial.close(handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_serial_close);

int32_t lpf_soc_serial_write(lpf_serial_handle_t handle, const void *buffer,
			     uint32_t size, int32_t timeout)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->serial.write)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->serial.write(handle, buffer, size, timeout);
}
EXPORT_SYMBOL_GPL(lpf_soc_serial_write);

int32_t lpf_soc_serial_read(lpf_serial_handle_t handle, void *buffer,
			    uint32_t size, int32_t timeout)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->serial.read)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->serial.read(handle, buffer, size, timeout);
}
EXPORT_SYMBOL_GPL(lpf_soc_serial_read);

int32_t lpf_soc_serial_flush(lpf_serial_handle_t handle)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->serial.flush)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->serial.flush(handle);
}
EXPORT_SYMBOL_GPL(lpf_soc_serial_flush);

int32_t lpf_soc_serial_set_config(lpf_serial_handle_t handle,
				  const lpf_serial_config_t *config)
{
	const lpf_soc_adapter_t *adapter = lpf_soc_get_adapter();

	if (!adapter || !adapter->serial.set_config)
		return OSAL_ERR_NOT_SUPPORTED;

	return adapter->serial.set_config(handle, config);
}
EXPORT_SYMBOL_GPL(lpf_soc_serial_set_config);
