// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "lpf/soc/lpf_soc_adapter.h"

#define LPF_SOC_MOCK_MAX_GPIO 256U
#define LPF_SOC_MOCK_MAX_PWM 32U
#define LPF_SOC_MOCK_MAX_I2C 16U
#define LPF_SOC_MOCK_MAX_SPI 16U
#define LPF_SOC_MOCK_MAX_CAN 16U
#define LPF_SOC_MOCK_MAX_SERIAL 16U
#define LPF_SOC_MOCK_SERIAL_BUFFER_SIZE 256U

typedef struct {
	uint32_t gpio_num;
	lpf_gpio_direction_t direction;
	lpf_gpio_level_t level;
	bool requested;
} lpf_soc_mock_gpio_t;

typedef struct {
	uint32_t gpio_num;
	lpf_gpio_edge_t edge;
	lpf_gpio_irq_callback_t callback;
	void *user_data;
	bool enabled;
	bool allocated;
} lpf_soc_mock_gpio_irq_t;

typedef struct {
	lpf_pwm_state_t state;
	char consumer[32];
	bool allocated;
} lpf_soc_mock_pwm_t;

typedef struct {
	char device[32];
	bool allocated;
} lpf_soc_mock_i2c_t;

typedef struct {
	lpf_spi_config_t config;
	char device[32];
	bool allocated;
} lpf_soc_mock_spi_t;

typedef struct {
	lpf_can_config_t config;
	char interface[32];
	lpf_can_frame_t last_tx;
	bool has_last_tx;
	bool allocated;
} lpf_soc_mock_can_t;

typedef struct {
	lpf_serial_config_t config;
	char device[32];
	uint8_t buffer[LPF_SOC_MOCK_SERIAL_BUFFER_SIZE];
	uint32_t buffer_len;
	bool allocated;
} lpf_soc_mock_serial_t;

static DEFINE_MUTEX(g_lpf_soc_mock_lock);
static lpf_soc_mock_gpio_t g_lpf_soc_mock_gpios[LPF_SOC_MOCK_MAX_GPIO];
static lpf_soc_mock_gpio_irq_t g_lpf_soc_mock_irqs[LPF_SOC_MOCK_MAX_GPIO];
static lpf_soc_mock_pwm_t g_lpf_soc_mock_pwms[LPF_SOC_MOCK_MAX_PWM];
static lpf_soc_mock_i2c_t g_lpf_soc_mock_i2c[LPF_SOC_MOCK_MAX_I2C];
static lpf_soc_mock_spi_t g_lpf_soc_mock_spi[LPF_SOC_MOCK_MAX_SPI];
static lpf_soc_mock_can_t g_lpf_soc_mock_can[LPF_SOC_MOCK_MAX_CAN];
static lpf_soc_mock_serial_t g_lpf_soc_mock_serial[LPF_SOC_MOCK_MAX_SERIAL];

static bool lpf_soc_mock_valid_gpio_level(lpf_gpio_level_t level)
{
	return level == LPF_GPIO_LEVEL_LOW || level == LPF_GPIO_LEVEL_HIGH;
}

static bool lpf_soc_mock_valid_gpio_direction(lpf_gpio_direction_t direction)
{
	return direction == LPF_GPIO_DIR_INPUT ||
	       direction == LPF_GPIO_DIR_OUTPUT;
}

static int32_t lpf_soc_mock_copy_name(char *dst, size_t dst_size,
				      const char *src)
{
	int len;

	if (!dst || dst_size == 0 || !src || src[0] == '\0')
		return OSAL_ERR_INVALID_PARAM;

	len = osal_snprintf(dst, dst_size, "%s", src);
	if (len <= 0 || len >= dst_size)
		return OSAL_ERR_INVALID_PARAM;

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_gpio_request(uint32_t gpio_num, const char *label)
{
	lpf_soc_mock_gpio_t *gpio;

	(void)label;

	if (gpio_num >= LPF_SOC_MOCK_MAX_GPIO)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	gpio = &g_lpf_soc_mock_gpios[gpio_num];
	if (gpio->requested) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_ALREADY_EXISTS;
	}

	gpio->gpio_num = gpio_num;
	gpio->direction = LPF_GPIO_DIR_INPUT;
	gpio->level = LPF_GPIO_LEVEL_LOW;
	gpio->requested = true;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static void lpf_soc_mock_gpio_free(uint32_t gpio_num)
{
	if (gpio_num >= LPF_SOC_MOCK_MAX_GPIO)
		return;

	mutex_lock(&g_lpf_soc_mock_lock);
	osal_memset(&g_lpf_soc_mock_gpios[gpio_num], 0,
		    sizeof(g_lpf_soc_mock_gpios[gpio_num]));
	osal_memset(&g_lpf_soc_mock_irqs[gpio_num], 0,
		    sizeof(g_lpf_soc_mock_irqs[gpio_num]));
	mutex_unlock(&g_lpf_soc_mock_lock);
}

static int32_t lpf_soc_mock_gpio_set_direction(
	uint32_t gpio_num, lpf_gpio_direction_t direction,
	lpf_gpio_level_t initial_level)
{
	lpf_soc_mock_gpio_t *gpio;

	if (gpio_num >= LPF_SOC_MOCK_MAX_GPIO ||
	    !lpf_soc_mock_valid_gpio_direction(direction) ||
	    !lpf_soc_mock_valid_gpio_level(initial_level))
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	gpio = &g_lpf_soc_mock_gpios[gpio_num];
	if (!gpio->requested) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_ID;
	}

	gpio->direction = direction;
	if (direction == LPF_GPIO_DIR_OUTPUT)
		gpio->level = initial_level;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_gpio_set_level(uint32_t gpio_num,
					   lpf_gpio_level_t level)
{
	lpf_soc_mock_gpio_irq_t *irq;
	lpf_soc_mock_gpio_t *gpio;
	lpf_gpio_irq_callback_t callback = NULL;
	void *user_data = NULL;
	bool should_notify = false;

	if (gpio_num >= LPF_SOC_MOCK_MAX_GPIO ||
	    !lpf_soc_mock_valid_gpio_level(level))
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	gpio = &g_lpf_soc_mock_gpios[gpio_num];
	if (!gpio->requested) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_ID;
	}

	if (gpio->level != level) {
		irq = &g_lpf_soc_mock_irqs[gpio_num];
		should_notify = irq->allocated && irq->enabled &&
				((level == LPF_GPIO_LEVEL_HIGH &&
				  (irq->edge == LPF_GPIO_EDGE_RISING ||
				   irq->edge == LPF_GPIO_EDGE_BOTH)) ||
				 (level == LPF_GPIO_LEVEL_LOW &&
				  (irq->edge == LPF_GPIO_EDGE_FALLING ||
				   irq->edge == LPF_GPIO_EDGE_BOTH)));
		if (should_notify) {
			callback = irq->callback;
			user_data = irq->user_data;
		}
	}

	gpio->level = level;
	mutex_unlock(&g_lpf_soc_mock_lock);

	if (callback)
		callback(gpio_num, level, user_data);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_gpio_get_level(uint32_t gpio_num,
					   lpf_gpio_level_t *level)
{
	if (gpio_num >= LPF_SOC_MOCK_MAX_GPIO || !level)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	if (!g_lpf_soc_mock_gpios[gpio_num].requested) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_ID;
	}

	*level = g_lpf_soc_mock_gpios[gpio_num].level;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_gpio_request_interrupt(
	uint32_t gpio_num, lpf_gpio_edge_t edge,
	lpf_gpio_irq_callback_t callback, void *user_data, void **irq_handle)
{
	lpf_soc_mock_gpio_irq_t *irq;

	if (gpio_num >= LPF_SOC_MOCK_MAX_GPIO || !callback || !irq_handle ||
	    edge == LPF_GPIO_EDGE_NONE)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	if (!g_lpf_soc_mock_gpios[gpio_num].requested) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_ID;
	}

	irq = &g_lpf_soc_mock_irqs[gpio_num];
	if (irq->allocated) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_ALREADY_EXISTS;
	}

	irq->gpio_num = gpio_num;
	irq->edge = edge;
	irq->callback = callback;
	irq->user_data = user_data;
	irq->enabled = true;
	irq->allocated = true;
	*irq_handle = irq;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static void lpf_soc_mock_gpio_free_interrupt(void *irq_handle)
{
	lpf_soc_mock_gpio_irq_t *irq = irq_handle;

	if (!irq)
		return;

	mutex_lock(&g_lpf_soc_mock_lock);
	osal_memset(irq, 0, sizeof(*irq));
	mutex_unlock(&g_lpf_soc_mock_lock);
}

static int32_t lpf_soc_mock_gpio_enable_interrupt(void *irq_handle)
{
	lpf_soc_mock_gpio_irq_t *irq = irq_handle;

	if (!irq || !irq->allocated)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	irq->enabled = true;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_gpio_disable_interrupt(void *irq_handle)
{
	lpf_soc_mock_gpio_irq_t *irq = irq_handle;

	if (!irq || !irq->allocated)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	irq->enabled = false;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_pwm_get(const char *consumer,
				    lpf_pwm_handle_t *handle)
{
	uint32_t i;
	int32_t ret;

	if (!consumer || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;

	mutex_lock(&g_lpf_soc_mock_lock);
	for (i = 0; i < LPF_SOC_MOCK_MAX_PWM; i++) {
		if (g_lpf_soc_mock_pwms[i].allocated)
			continue;

		ret = lpf_soc_mock_copy_name(g_lpf_soc_mock_pwms[i].consumer,
					     sizeof(g_lpf_soc_mock_pwms[i].consumer),
					     consumer);
		if (ret != OSAL_SUCCESS) {
			mutex_unlock(&g_lpf_soc_mock_lock);
			return ret;
		}

		g_lpf_soc_mock_pwms[i].allocated = true;
		*handle = &g_lpf_soc_mock_pwms[i];
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_SUCCESS;
	}
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_ERR_RESOURCE_LIMIT;
}

static void lpf_soc_mock_pwm_put(lpf_pwm_handle_t handle)
{
	lpf_soc_mock_pwm_t *pwm = handle;

	if (!pwm)
		return;

	mutex_lock(&g_lpf_soc_mock_lock);
	osal_memset(pwm, 0, sizeof(*pwm));
	mutex_unlock(&g_lpf_soc_mock_lock);
}

static int32_t lpf_soc_mock_pwm_apply(lpf_pwm_handle_t handle,
				      const lpf_pwm_state_t *state)
{
	lpf_soc_mock_pwm_t *pwm = handle;

	if (!pwm || !state || !pwm->allocated || state->period_ns == 0 ||
	    state->duty_ns > state->period_ns)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	pwm->state = *state;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_pwm_get_state(lpf_pwm_handle_t handle,
					  lpf_pwm_state_t *state)
{
	lpf_soc_mock_pwm_t *pwm = handle;

	if (!pwm || !state || !pwm->allocated)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	*state = pwm->state;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_pwm_enable(lpf_pwm_handle_t handle)
{
	lpf_soc_mock_pwm_t *pwm = handle;

	if (!pwm || !pwm->allocated)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	pwm->state.enabled = true;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_pwm_disable(lpf_pwm_handle_t handle)
{
	lpf_soc_mock_pwm_t *pwm = handle;

	if (!pwm || !pwm->allocated)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	pwm->state.enabled = false;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_i2c_open(const char *device,
				     lpf_i2c_handle_t *handle)
{
	uint32_t i;
	int32_t ret;

	if (!device || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;

	mutex_lock(&g_lpf_soc_mock_lock);
	for (i = 0; i < LPF_SOC_MOCK_MAX_I2C; i++) {
		if (g_lpf_soc_mock_i2c[i].allocated)
			continue;

		ret = lpf_soc_mock_copy_name(g_lpf_soc_mock_i2c[i].device,
					     sizeof(g_lpf_soc_mock_i2c[i].device),
					     device);
		if (ret != OSAL_SUCCESS) {
			mutex_unlock(&g_lpf_soc_mock_lock);
			return ret;
		}

		g_lpf_soc_mock_i2c[i].allocated = true;
		*handle = &g_lpf_soc_mock_i2c[i];
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_SUCCESS;
	}
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_ERR_RESOURCE_LIMIT;
}

static void lpf_soc_mock_i2c_close(lpf_i2c_handle_t handle)
{
	lpf_soc_mock_i2c_t *i2c = handle;

	if (!i2c)
		return;

	mutex_lock(&g_lpf_soc_mock_lock);
	osal_memset(i2c, 0, sizeof(*i2c));
	mutex_unlock(&g_lpf_soc_mock_lock);
}

static int32_t lpf_soc_mock_i2c_transfer(lpf_i2c_handle_t handle,
					 lpf_i2c_msg_t *msgs, uint32_t num)
{
	lpf_soc_mock_i2c_t *i2c = handle;
	uint32_t i;
	uint16_t j;

	if (!i2c || !i2c->allocated || !msgs || num == 0 || num > 64)
		return OSAL_ERR_INVALID_PARAM;

	for (i = 0; i < num; i++) {
		if (!msgs[i].buf || msgs[i].len == 0)
			return OSAL_ERR_INVALID_PARAM;

		if (!(msgs[i].flags & LPF_I2C_M_RD))
			continue;

		for (j = 0; j < msgs[i].len; j++)
			msgs[i].buf[j] = (uint8_t)j;
	}

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_spi_open(const lpf_spi_config_t *config,
				     lpf_spi_handle_t *handle)
{
	uint32_t i;
	int32_t ret;

	if (!config || !config->device || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;

	mutex_lock(&g_lpf_soc_mock_lock);
	for (i = 0; i < LPF_SOC_MOCK_MAX_SPI; i++) {
		if (g_lpf_soc_mock_spi[i].allocated)
			continue;

		ret = lpf_soc_mock_copy_name(g_lpf_soc_mock_spi[i].device,
					     sizeof(g_lpf_soc_mock_spi[i].device),
					     config->device);
		if (ret != OSAL_SUCCESS) {
			mutex_unlock(&g_lpf_soc_mock_lock);
			return ret;
		}

		g_lpf_soc_mock_spi[i].config = *config;
		g_lpf_soc_mock_spi[i].config.device =
			g_lpf_soc_mock_spi[i].device;
		g_lpf_soc_mock_spi[i].allocated = true;
		*handle = &g_lpf_soc_mock_spi[i];
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_SUCCESS;
	}
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_ERR_RESOURCE_LIMIT;
}

static void lpf_soc_mock_spi_close(lpf_spi_handle_t handle)
{
	lpf_soc_mock_spi_t *spi = handle;

	if (!spi)
		return;

	mutex_lock(&g_lpf_soc_mock_lock);
	osal_memset(spi, 0, sizeof(*spi));
	mutex_unlock(&g_lpf_soc_mock_lock);
}

static int32_t lpf_soc_mock_spi_transfer(
	lpf_spi_handle_t handle, const lpf_spi_transfer_t *transfers,
	uint32_t num)
{
	lpf_soc_mock_spi_t *spi = handle;
	uint32_t i;

	if (!spi || !spi->allocated || !transfers || num == 0)
		return OSAL_ERR_INVALID_PARAM;

	for (i = 0; i < num; i++) {
		if ((!transfers[i].tx_buf && !transfers[i].rx_buf) ||
		    transfers[i].len == 0)
			return OSAL_ERR_INVALID_PARAM;

		if (transfers[i].rx_buf && transfers[i].tx_buf)
			osal_memcpy(transfers[i].rx_buf, transfers[i].tx_buf,
				    transfers[i].len);
		else if (transfers[i].rx_buf)
			osal_memset(transfers[i].rx_buf, 0,
				    transfers[i].len);
	}

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_spi_set_config(lpf_spi_handle_t handle,
					   const lpf_spi_config_t *config)
{
	lpf_soc_mock_spi_t *spi = handle;

	if (!spi || !spi->allocated || !config || !config->device)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	spi->config = *config;
	spi->config.device = spi->device;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_can_init(const lpf_can_config_t *config,
				     lpf_can_handle_t *handle)
{
	uint32_t i;
	int32_t ret;

	if (!config || !config->interface || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;

	mutex_lock(&g_lpf_soc_mock_lock);
	for (i = 0; i < LPF_SOC_MOCK_MAX_CAN; i++) {
		if (g_lpf_soc_mock_can[i].allocated)
			continue;

		ret = lpf_soc_mock_copy_name(g_lpf_soc_mock_can[i].interface,
					     sizeof(g_lpf_soc_mock_can[i].interface),
					     config->interface);
		if (ret != OSAL_SUCCESS) {
			mutex_unlock(&g_lpf_soc_mock_lock);
			return ret;
		}

		g_lpf_soc_mock_can[i].config = *config;
		g_lpf_soc_mock_can[i].config.interface =
			g_lpf_soc_mock_can[i].interface;
		g_lpf_soc_mock_can[i].allocated = true;
		*handle = &g_lpf_soc_mock_can[i];
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_SUCCESS;
	}
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_ERR_RESOURCE_LIMIT;
}

static int32_t lpf_soc_mock_can_deinit(lpf_can_handle_t handle)
{
	lpf_soc_mock_can_t *can = handle;

	if (!can)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	osal_memset(can, 0, sizeof(*can));
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_can_send(lpf_can_handle_t handle,
				     const lpf_can_frame_t *frame)
{
	lpf_soc_mock_can_t *can = handle;

	if (!can || !can->allocated || !frame ||
	    frame->dlc > LPF_CAN_MAX_DATA_LEN)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	can->last_tx = *frame;
	can->has_last_tx = true;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_can_recv(lpf_can_handle_t handle,
				     lpf_can_frame_t *frame,
				     int32_t timeout)
{
	lpf_soc_mock_can_t *can = handle;

	(void)timeout;

	if (!can || !can->allocated || !frame)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	if (!can->has_last_tx) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_TIMEOUT;
	}

	*frame = can->last_tx;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_can_set_filter(lpf_can_handle_t handle,
					   uint32_t filter_id,
					   uint32_t filter_mask)
{
	lpf_soc_mock_can_t *can = handle;

	(void)filter_id;
	(void)filter_mask;

	if (!can || !can->allocated)
		return OSAL_ERR_INVALID_PARAM;

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_serial_open(
	const char *device, const lpf_serial_config_t *config,
	lpf_serial_handle_t *handle)
{
	uint32_t i;
	int32_t ret;

	if (!device || !config || !handle || config->baud_rate == 0)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;

	mutex_lock(&g_lpf_soc_mock_lock);
	for (i = 0; i < LPF_SOC_MOCK_MAX_SERIAL; i++) {
		if (g_lpf_soc_mock_serial[i].allocated)
			continue;

		ret = lpf_soc_mock_copy_name(g_lpf_soc_mock_serial[i].device,
					     sizeof(g_lpf_soc_mock_serial[i].device),
					     device);
		if (ret != OSAL_SUCCESS) {
			mutex_unlock(&g_lpf_soc_mock_lock);
			return ret;
		}

		g_lpf_soc_mock_serial[i].config = *config;
		g_lpf_soc_mock_serial[i].allocated = true;
		*handle = &g_lpf_soc_mock_serial[i];
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_SUCCESS;
	}
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_ERR_RESOURCE_LIMIT;
}

static int32_t lpf_soc_mock_serial_close(lpf_serial_handle_t handle)
{
	lpf_soc_mock_serial_t *serial = handle;

	if (!serial)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	osal_memset(serial, 0, sizeof(*serial));
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_serial_write(lpf_serial_handle_t handle,
					 const void *buffer, uint32_t size,
					 int32_t timeout)
{
	lpf_soc_mock_serial_t *serial = handle;
	uint32_t copy_len;

	(void)timeout;

	if (!serial || !serial->allocated || !buffer || size == 0)
		return OSAL_ERR_INVALID_PARAM;

	copy_len = min_t(uint32_t, size, sizeof(serial->buffer));

	mutex_lock(&g_lpf_soc_mock_lock);
	osal_memcpy(serial->buffer, buffer, copy_len);
	serial->buffer_len = copy_len;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return (int32_t)size;
}

static int32_t lpf_soc_mock_serial_read(lpf_serial_handle_t handle,
					void *buffer, uint32_t size,
					int32_t timeout)
{
	lpf_soc_mock_serial_t *serial = handle;
	uint32_t copy_len;

	(void)timeout;

	if (!serial || !serial->allocated || !buffer || size == 0)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	copy_len = min_t(uint32_t, size, serial->buffer_len);
	if (copy_len > 0)
		osal_memcpy(buffer, serial->buffer, copy_len);
	mutex_unlock(&g_lpf_soc_mock_lock);

	return (int32_t)copy_len;
}

static int32_t lpf_soc_mock_serial_flush(lpf_serial_handle_t handle)
{
	lpf_soc_mock_serial_t *serial = handle;

	if (!serial || !serial->allocated)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	serial->buffer_len = 0;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_serial_set_config(
	lpf_serial_handle_t handle, const lpf_serial_config_t *config)
{
	lpf_soc_mock_serial_t *serial = handle;

	if (!serial || !serial->allocated || !config ||
	    config->baud_rate == 0)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	serial->config = *config;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

const lpf_soc_adapter_t g_lpf_soc_mock_adapter = {
	.name = "mock",
	.gpio = {
		.request = lpf_soc_mock_gpio_request,
		.free = lpf_soc_mock_gpio_free,
		.set_direction = lpf_soc_mock_gpio_set_direction,
		.set_level = lpf_soc_mock_gpio_set_level,
		.get_level = lpf_soc_mock_gpio_get_level,
		.request_interrupt = lpf_soc_mock_gpio_request_interrupt,
		.free_interrupt = lpf_soc_mock_gpio_free_interrupt,
		.enable_interrupt = lpf_soc_mock_gpio_enable_interrupt,
		.disable_interrupt = lpf_soc_mock_gpio_disable_interrupt,
	},
	.pwm = {
		.get = lpf_soc_mock_pwm_get,
		.put = lpf_soc_mock_pwm_put,
		.apply = lpf_soc_mock_pwm_apply,
		.get_state = lpf_soc_mock_pwm_get_state,
		.enable = lpf_soc_mock_pwm_enable,
		.disable = lpf_soc_mock_pwm_disable,
	},
	.i2c = {
		.open = lpf_soc_mock_i2c_open,
		.close = lpf_soc_mock_i2c_close,
		.transfer = lpf_soc_mock_i2c_transfer,
	},
	.spi = {
		.open = lpf_soc_mock_spi_open,
		.close = lpf_soc_mock_spi_close,
		.transfer = lpf_soc_mock_spi_transfer,
		.set_config = lpf_soc_mock_spi_set_config,
	},
	.can = {
		.init = lpf_soc_mock_can_init,
		.deinit = lpf_soc_mock_can_deinit,
		.send = lpf_soc_mock_can_send,
		.recv = lpf_soc_mock_can_recv,
		.set_filter = lpf_soc_mock_can_set_filter,
	},
	.serial = {
		.open = lpf_soc_mock_serial_open,
		.close = lpf_soc_mock_serial_close,
		.write = lpf_soc_mock_serial_write,
		.read = lpf_soc_mock_serial_read,
		.flush = lpf_soc_mock_serial_flush,
		.set_config = lpf_soc_mock_serial_set_config,
	},
};
