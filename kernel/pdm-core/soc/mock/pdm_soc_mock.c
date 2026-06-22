// SPDX-License-Identifier: GPL-2.0

#include <linux/list.h>
#include <linux/module.h>

#include "osal.h"
#include "lpf/soc/lpf_soc_adapter.h"

#define LPF_SOC_MOCK_SERIAL_BUFFER_SIZE 256U

typedef struct {
	uint32_t gpio_num;
	lpf_gpio_edge_t edge;
	lpf_gpio_irq_callback_t callback;
	void *user_data;
	bool enabled;
	bool allocated;
} lpf_soc_mock_gpio_irq_t;

typedef struct {
	uint32_t gpio_num;
	lpf_gpio_direction_t direction;
	lpf_gpio_level_t level;
	bool requested;
	lpf_soc_mock_gpio_irq_t irq;
	struct list_head node;
} lpf_soc_mock_gpio_t;

typedef struct {
	struct list_head node;
	lpf_pwm_state_t state;
	char consumer[32];
	bool allocated;
} lpf_soc_mock_pwm_t;

typedef struct {
	struct list_head node;
	char device[32];
	bool allocated;
} lpf_soc_mock_i2c_t;

typedef struct {
	struct list_head node;
	lpf_spi_config_t config;
	char device[32];
	bool allocated;
} lpf_soc_mock_spi_t;

typedef struct {
	struct list_head node;
	lpf_can_config_t config;
	char interface[32];
	lpf_can_frame_t last_tx;
	bool has_last_tx;
	bool allocated;
} lpf_soc_mock_can_t;

typedef struct {
	struct list_head node;
	lpf_serial_config_t config;
	char device[32];
	uint8_t buffer[LPF_SOC_MOCK_SERIAL_BUFFER_SIZE];
	uint32_t buffer_len;
	bool allocated;
} lpf_soc_mock_serial_t;

static DEFINE_MUTEX(g_lpf_soc_mock_lock);
static LIST_HEAD(g_lpf_soc_mock_gpios);
static LIST_HEAD(g_lpf_soc_mock_pwms);
static LIST_HEAD(g_lpf_soc_mock_i2c);
static LIST_HEAD(g_lpf_soc_mock_spi);
static LIST_HEAD(g_lpf_soc_mock_can);
static LIST_HEAD(g_lpf_soc_mock_serial);

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

static lpf_soc_mock_gpio_t *lpf_soc_mock_find_gpio_locked(uint32_t gpio_num)
{
	lpf_soc_mock_gpio_t *gpio;

	list_for_each_entry(gpio, &g_lpf_soc_mock_gpios, node) {
		if (gpio->gpio_num == gpio_num)
			return gpio;
	}

	return NULL;
}

static lpf_soc_mock_gpio_irq_t *
lpf_soc_mock_find_gpio_irq_locked(void *irq_handle)
{
	lpf_soc_mock_gpio_t *gpio;

	if (!irq_handle)
		return NULL;

	list_for_each_entry(gpio, &g_lpf_soc_mock_gpios, node) {
		if (&gpio->irq == irq_handle && gpio->irq.allocated)
			return &gpio->irq;
	}

	return NULL;
}

static lpf_soc_mock_pwm_t *
lpf_soc_mock_find_pwm_locked(lpf_pwm_handle_t handle)
{
	lpf_soc_mock_pwm_t *pwm;

	if (!handle)
		return NULL;

	list_for_each_entry(pwm, &g_lpf_soc_mock_pwms, node) {
		if ((lpf_pwm_handle_t)pwm == handle && pwm->allocated)
			return pwm;
	}

	return NULL;
}

static lpf_soc_mock_i2c_t *
lpf_soc_mock_find_i2c_locked(lpf_i2c_handle_t handle)
{
	lpf_soc_mock_i2c_t *i2c;

	if (!handle)
		return NULL;

	list_for_each_entry(i2c, &g_lpf_soc_mock_i2c, node) {
		if ((lpf_i2c_handle_t)i2c == handle && i2c->allocated)
			return i2c;
	}

	return NULL;
}

static lpf_soc_mock_spi_t *
lpf_soc_mock_find_spi_locked(lpf_spi_handle_t handle)
{
	lpf_soc_mock_spi_t *spi;

	if (!handle)
		return NULL;

	list_for_each_entry(spi, &g_lpf_soc_mock_spi, node) {
		if ((lpf_spi_handle_t)spi == handle && spi->allocated)
			return spi;
	}

	return NULL;
}

static lpf_soc_mock_can_t *
lpf_soc_mock_find_can_locked(lpf_can_handle_t handle)
{
	lpf_soc_mock_can_t *can;

	if (!handle)
		return NULL;

	list_for_each_entry(can, &g_lpf_soc_mock_can, node) {
		if ((lpf_can_handle_t)can == handle && can->allocated)
			return can;
	}

	return NULL;
}

static lpf_soc_mock_serial_t *
lpf_soc_mock_find_serial_locked(lpf_serial_handle_t handle)
{
	lpf_soc_mock_serial_t *serial;

	if (!handle)
		return NULL;

	list_for_each_entry(serial, &g_lpf_soc_mock_serial, node) {
		if ((lpf_serial_handle_t)serial == handle &&
		    serial->allocated)
			return serial;
	}

	return NULL;
}

static int32_t lpf_soc_mock_gpio_request(uint32_t gpio_num, const char *label)
{
	lpf_soc_mock_gpio_t *gpio;

	(void)label;

	gpio = osal_zalloc(sizeof(*gpio));
	if (!gpio)
		return OSAL_ERR_NO_MEMORY;

	mutex_lock(&g_lpf_soc_mock_lock);
	if (lpf_soc_mock_find_gpio_locked(gpio_num)) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		osal_free(gpio);
		return OSAL_ERR_ALREADY_EXISTS;
	}

	gpio->gpio_num = gpio_num;
	gpio->direction = LPF_GPIO_DIR_INPUT;
	gpio->level = LPF_GPIO_LEVEL_LOW;
	gpio->requested = true;
	INIT_LIST_HEAD(&gpio->node);
	list_add(&gpio->node, &g_lpf_soc_mock_gpios);
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static void lpf_soc_mock_gpio_free(uint32_t gpio_num)
{
	lpf_soc_mock_gpio_t *gpio;

	mutex_lock(&g_lpf_soc_mock_lock);
	gpio = lpf_soc_mock_find_gpio_locked(gpio_num);
	if (gpio)
		list_del_init(&gpio->node);
	mutex_unlock(&g_lpf_soc_mock_lock);

	osal_free(gpio);
}

static int32_t lpf_soc_mock_gpio_set_direction(
	uint32_t gpio_num, lpf_gpio_direction_t direction,
	lpf_gpio_level_t initial_level)
{
	lpf_soc_mock_gpio_t *gpio;

	if (!lpf_soc_mock_valid_gpio_direction(direction) ||
	    !lpf_soc_mock_valid_gpio_level(initial_level))
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	gpio = lpf_soc_mock_find_gpio_locked(gpio_num);
	if (!gpio || !gpio->requested) {
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

	if (!lpf_soc_mock_valid_gpio_level(level))
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	gpio = lpf_soc_mock_find_gpio_locked(gpio_num);
	if (!gpio || !gpio->requested) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_ID;
	}

	if (gpio->level != level) {
		irq = &gpio->irq;
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
	lpf_soc_mock_gpio_t *gpio;

	if (!level)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	gpio = lpf_soc_mock_find_gpio_locked(gpio_num);
	if (!gpio || !gpio->requested) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_ID;
	}

	*level = gpio->level;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_gpio_request_interrupt(
	uint32_t gpio_num, lpf_gpio_edge_t edge,
	lpf_gpio_irq_callback_t callback, void *user_data, void **irq_handle)
{
	lpf_soc_mock_gpio_irq_t *irq;
	lpf_soc_mock_gpio_t *gpio;

	if (!callback || !irq_handle || edge == LPF_GPIO_EDGE_NONE)
		return OSAL_ERR_INVALID_PARAM;

	*irq_handle = NULL;

	mutex_lock(&g_lpf_soc_mock_lock);
	gpio = lpf_soc_mock_find_gpio_locked(gpio_num);
	if (!gpio || !gpio->requested) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_ID;
	}

	irq = &gpio->irq;
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
	lpf_soc_mock_gpio_irq_t *irq;

	mutex_lock(&g_lpf_soc_mock_lock);
	irq = lpf_soc_mock_find_gpio_irq_locked(irq_handle);
	if (irq)
		osal_memset(irq, 0, sizeof(*irq));
	mutex_unlock(&g_lpf_soc_mock_lock);
}

static int32_t lpf_soc_mock_gpio_enable_interrupt(void *irq_handle)
{
	lpf_soc_mock_gpio_irq_t *irq;

	mutex_lock(&g_lpf_soc_mock_lock);
	irq = lpf_soc_mock_find_gpio_irq_locked(irq_handle);
	if (!irq) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	irq->enabled = true;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_gpio_disable_interrupt(void *irq_handle)
{
	lpf_soc_mock_gpio_irq_t *irq;

	mutex_lock(&g_lpf_soc_mock_lock);
	irq = lpf_soc_mock_find_gpio_irq_locked(irq_handle);
	if (!irq) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	irq->enabled = false;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_pwm_get(const char *consumer,
				    lpf_pwm_handle_t *handle)
{
	lpf_soc_mock_pwm_t *pwm;
	int32_t ret;

	if (!consumer || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;
	pwm = osal_zalloc(sizeof(*pwm));
	if (!pwm)
		return OSAL_ERR_NO_MEMORY;

	ret = lpf_soc_mock_copy_name(pwm->consumer, sizeof(pwm->consumer),
				     consumer);
	if (ret != OSAL_SUCCESS) {
		osal_free(pwm);
		return ret;
	}

	pwm->allocated = true;
	INIT_LIST_HEAD(&pwm->node);
	mutex_lock(&g_lpf_soc_mock_lock);
	list_add(&pwm->node, &g_lpf_soc_mock_pwms);
	mutex_unlock(&g_lpf_soc_mock_lock);
	*handle = pwm;

	return OSAL_SUCCESS;
}

static void lpf_soc_mock_pwm_put(lpf_pwm_handle_t handle)
{
	lpf_soc_mock_pwm_t *pwm;

	mutex_lock(&g_lpf_soc_mock_lock);
	pwm = lpf_soc_mock_find_pwm_locked(handle);
	if (pwm)
		list_del_init(&pwm->node);
	mutex_unlock(&g_lpf_soc_mock_lock);

	osal_free(pwm);
}

static int32_t lpf_soc_mock_pwm_apply(lpf_pwm_handle_t handle,
				      const lpf_pwm_state_t *state)
{
	lpf_soc_mock_pwm_t *pwm;

	if (!state || state->period_ns == 0 ||
	    state->duty_ns > state->period_ns)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	pwm = lpf_soc_mock_find_pwm_locked(handle);
	if (!pwm) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	pwm->state = *state;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_pwm_get_state(lpf_pwm_handle_t handle,
					  lpf_pwm_state_t *state)
{
	lpf_soc_mock_pwm_t *pwm;

	if (!state)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	pwm = lpf_soc_mock_find_pwm_locked(handle);
	if (!pwm) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	*state = pwm->state;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_pwm_enable(lpf_pwm_handle_t handle)
{
	lpf_soc_mock_pwm_t *pwm;

	mutex_lock(&g_lpf_soc_mock_lock);
	pwm = lpf_soc_mock_find_pwm_locked(handle);
	if (!pwm) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	pwm->state.enabled = true;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_pwm_disable(lpf_pwm_handle_t handle)
{
	lpf_soc_mock_pwm_t *pwm;

	mutex_lock(&g_lpf_soc_mock_lock);
	pwm = lpf_soc_mock_find_pwm_locked(handle);
	if (!pwm) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	pwm->state.enabled = false;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_i2c_open(const char *device,
				     lpf_i2c_handle_t *handle)
{
	lpf_soc_mock_i2c_t *i2c;
	int32_t ret;

	if (!device || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;
	i2c = osal_zalloc(sizeof(*i2c));
	if (!i2c)
		return OSAL_ERR_NO_MEMORY;

	ret = lpf_soc_mock_copy_name(i2c->device, sizeof(i2c->device),
				     device);
	if (ret != OSAL_SUCCESS) {
		osal_free(i2c);
		return ret;
	}

	i2c->allocated = true;
	INIT_LIST_HEAD(&i2c->node);
	mutex_lock(&g_lpf_soc_mock_lock);
	list_add(&i2c->node, &g_lpf_soc_mock_i2c);
	mutex_unlock(&g_lpf_soc_mock_lock);
	*handle = i2c;

	return OSAL_SUCCESS;
}

static void lpf_soc_mock_i2c_close(lpf_i2c_handle_t handle)
{
	lpf_soc_mock_i2c_t *i2c;

	mutex_lock(&g_lpf_soc_mock_lock);
	i2c = lpf_soc_mock_find_i2c_locked(handle);
	if (i2c)
		list_del_init(&i2c->node);
	mutex_unlock(&g_lpf_soc_mock_lock);

	osal_free(i2c);
}

static int32_t lpf_soc_mock_i2c_transfer(lpf_i2c_handle_t handle,
					 lpf_i2c_msg_t *msgs, uint32_t num)
{
	uint32_t i;
	uint16_t j;

	if (!msgs || num == 0 || num > 64)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	if (!lpf_soc_mock_find_i2c_locked(handle)) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}
	mutex_unlock(&g_lpf_soc_mock_lock);

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
	lpf_soc_mock_spi_t *spi;
	int32_t ret;

	if (!config || !config->device || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;
	spi = osal_zalloc(sizeof(*spi));
	if (!spi)
		return OSAL_ERR_NO_MEMORY;

	ret = lpf_soc_mock_copy_name(spi->device, sizeof(spi->device),
				     config->device);
	if (ret != OSAL_SUCCESS) {
		osal_free(spi);
		return ret;
	}

	spi->config = *config;
	spi->config.device = spi->device;
	spi->allocated = true;
	INIT_LIST_HEAD(&spi->node);
	mutex_lock(&g_lpf_soc_mock_lock);
	list_add(&spi->node, &g_lpf_soc_mock_spi);
	mutex_unlock(&g_lpf_soc_mock_lock);
	*handle = spi;

	return OSAL_SUCCESS;
}

static void lpf_soc_mock_spi_close(lpf_spi_handle_t handle)
{
	lpf_soc_mock_spi_t *spi;

	mutex_lock(&g_lpf_soc_mock_lock);
	spi = lpf_soc_mock_find_spi_locked(handle);
	if (spi)
		list_del_init(&spi->node);
	mutex_unlock(&g_lpf_soc_mock_lock);

	osal_free(spi);
}

static int32_t lpf_soc_mock_spi_transfer(
	lpf_spi_handle_t handle, const lpf_spi_transfer_t *transfers,
	uint32_t num)
{
	uint32_t i;

	if (!transfers || num == 0)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	if (!lpf_soc_mock_find_spi_locked(handle)) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	for (i = 0; i < num; i++) {
		if ((!transfers[i].tx_buf && !transfers[i].rx_buf) ||
		    transfers[i].len == 0) {
			mutex_unlock(&g_lpf_soc_mock_lock);
			return OSAL_ERR_INVALID_PARAM;
		}

		if (transfers[i].rx_buf && transfers[i].tx_buf)
			osal_memcpy(transfers[i].rx_buf, transfers[i].tx_buf,
				    transfers[i].len);
		else if (transfers[i].rx_buf)
			osal_memset(transfers[i].rx_buf, 0,
				    transfers[i].len);
	}
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_spi_set_config(lpf_spi_handle_t handle,
					   const lpf_spi_config_t *config)
{
	lpf_soc_mock_spi_t *spi;

	if (!config || !config->device)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	spi = lpf_soc_mock_find_spi_locked(handle);
	if (!spi) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	spi->config = *config;
	spi->config.device = spi->device;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_can_init(const lpf_can_config_t *config,
				     lpf_can_handle_t *handle)
{
	lpf_soc_mock_can_t *can;
	int32_t ret;

	if (!config || !config->interface || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;
	can = osal_zalloc(sizeof(*can));
	if (!can)
		return OSAL_ERR_NO_MEMORY;

	ret = lpf_soc_mock_copy_name(can->interface, sizeof(can->interface),
				     config->interface);
	if (ret != OSAL_SUCCESS) {
		osal_free(can);
		return ret;
	}

	can->config = *config;
	can->config.interface = can->interface;
	can->allocated = true;
	INIT_LIST_HEAD(&can->node);
	mutex_lock(&g_lpf_soc_mock_lock);
	list_add(&can->node, &g_lpf_soc_mock_can);
	mutex_unlock(&g_lpf_soc_mock_lock);
	*handle = can;

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_can_deinit(lpf_can_handle_t handle)
{
	lpf_soc_mock_can_t *can;

	mutex_lock(&g_lpf_soc_mock_lock);
	can = lpf_soc_mock_find_can_locked(handle);
	if (!can) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	list_del_init(&can->node);
	mutex_unlock(&g_lpf_soc_mock_lock);
	osal_free(can);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_can_send(lpf_can_handle_t handle,
				     const lpf_can_frame_t *frame)
{
	lpf_soc_mock_can_t *can;

	if (!frame || frame->dlc > LPF_CAN_MAX_DATA_LEN)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	can = lpf_soc_mock_find_can_locked(handle);
	if (!can) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	can->last_tx = *frame;
	can->has_last_tx = true;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_can_recv(lpf_can_handle_t handle,
				     lpf_can_frame_t *frame,
				     int32_t timeout)
{
	lpf_soc_mock_can_t *can;

	(void)timeout;

	if (!frame)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	can = lpf_soc_mock_find_can_locked(handle);
	if (!can) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

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
	(void)filter_id;
	(void)filter_mask;

	mutex_lock(&g_lpf_soc_mock_lock);
	if (!lpf_soc_mock_find_can_locked(handle)) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_serial_open(
	const char *device, const lpf_serial_config_t *config,
	lpf_serial_handle_t *handle)
{
	lpf_soc_mock_serial_t *serial;
	int32_t ret;

	if (!device || !config || !handle || config->baud_rate == 0)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;
	serial = osal_zalloc(sizeof(*serial));
	if (!serial)
		return OSAL_ERR_NO_MEMORY;

	ret = lpf_soc_mock_copy_name(serial->device, sizeof(serial->device),
				     device);
	if (ret != OSAL_SUCCESS) {
		osal_free(serial);
		return ret;
	}

	serial->config = *config;
	serial->allocated = true;
	INIT_LIST_HEAD(&serial->node);
	mutex_lock(&g_lpf_soc_mock_lock);
	list_add(&serial->node, &g_lpf_soc_mock_serial);
	mutex_unlock(&g_lpf_soc_mock_lock);
	*handle = serial;

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_serial_close(lpf_serial_handle_t handle)
{
	lpf_soc_mock_serial_t *serial;

	mutex_lock(&g_lpf_soc_mock_lock);
	serial = lpf_soc_mock_find_serial_locked(handle);
	if (!serial) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	list_del_init(&serial->node);
	mutex_unlock(&g_lpf_soc_mock_lock);
	osal_free(serial);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_serial_write(lpf_serial_handle_t handle,
					 const void *buffer, uint32_t size,
					 int32_t timeout)
{
	lpf_soc_mock_serial_t *serial;
	uint32_t copy_len;

	(void)timeout;

	if (!buffer || size == 0)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	serial = lpf_soc_mock_find_serial_locked(handle);
	if (!serial) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	copy_len = min_t(uint32_t, size, sizeof(serial->buffer));
	osal_memcpy(serial->buffer, buffer, copy_len);
	serial->buffer_len = copy_len;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return (int32_t)size;
}

static int32_t lpf_soc_mock_serial_read(lpf_serial_handle_t handle,
					void *buffer, uint32_t size,
					int32_t timeout)
{
	lpf_soc_mock_serial_t *serial;
	uint32_t copy_len;

	(void)timeout;

	if (!buffer || size == 0)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	serial = lpf_soc_mock_find_serial_locked(handle);
	if (!serial) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	copy_len = min_t(uint32_t, size, serial->buffer_len);
	if (copy_len > 0)
		osal_memcpy(buffer, serial->buffer, copy_len);
	mutex_unlock(&g_lpf_soc_mock_lock);

	return (int32_t)copy_len;
}

static int32_t lpf_soc_mock_serial_flush(lpf_serial_handle_t handle)
{
	lpf_soc_mock_serial_t *serial;

	mutex_lock(&g_lpf_soc_mock_lock);
	serial = lpf_soc_mock_find_serial_locked(handle);
	if (!serial) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

	serial->buffer_len = 0;
	mutex_unlock(&g_lpf_soc_mock_lock);

	return OSAL_SUCCESS;
}

static int32_t lpf_soc_mock_serial_set_config(
	lpf_serial_handle_t handle, const lpf_serial_config_t *config)
{
	lpf_soc_mock_serial_t *serial;

	if (!config || config->baud_rate == 0)
		return OSAL_ERR_INVALID_PARAM;

	mutex_lock(&g_lpf_soc_mock_lock);
	serial = lpf_soc_mock_find_serial_locked(handle);
	if (!serial) {
		mutex_unlock(&g_lpf_soc_mock_lock);
		return OSAL_ERR_INVALID_PARAM;
	}

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
