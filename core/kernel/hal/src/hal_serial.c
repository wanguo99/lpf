// SPDX-License-Identifier: GPL-2.0

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/termios.h>
#include <linux/tty.h>
#include <linux/tty_ldisc.h>
#include <linux/wait.h>

#include "osal.h"
#include "hal_serial.h"

typedef struct {
	struct file *file;
	struct tty_struct *tty;
	osal_mutex_t tx_lock;
	osal_mutex_t rx_lock;
	osal_mutex_t config_lock;
	osal_mutex_t state_lock;
	wait_queue_head_t idle_wait;
	hal_serial_config_t config;
	uint32_t active_ops;
	char path[128];
	bool initialized;
	bool closing;
} hal_serial_context_t;

static int32_t hal_serial_errno_to_status(int ret)
{
	int err;

	if (ret >= 0)
		return OSAL_SUCCESS;

	err = -ret;
	if (err == EAGAIN || err == EWOULDBLOCK || err == ETIMEDOUT)
		return OSAL_ERR_TIMEOUT;
	if (err == ENODEV || err == ENOENT)
		return OSAL_ENOENT;
	if (err == EOPNOTSUPP || err == ENOSYS)
		return OSAL_ERR_NOT_SUPPORTED;

	return err;
}

static bool hal_serial_valid_config(const hal_serial_config_t *config)
{
	if (!config)
		return false;

	if (config->baud_rate == 0)
		return false;

	if (config->data_bits < 5 || config->data_bits > 8)
		return false;

	if (config->stop_bits != 1 && config->stop_bits != 2)
		return false;

	if (config->parity != HAL_SERIAL_PARITY_NONE &&
	    config->parity != HAL_SERIAL_PARITY_ODD &&
	    config->parity != HAL_SERIAL_PARITY_EVEN)
		return false;

	if (config->flow_control != HAL_SERIAL_FLOW_NONE &&
	    config->flow_control != HAL_SERIAL_FLOW_HW &&
	    config->flow_control != HAL_SERIAL_FLOW_SW)
		return false;

	return true;
}

static int32_t hal_serial_build_path(const char *device, char *path,
				     size_t path_size)
{
	int len;

	if (!device || !path || path_size == 0 || device[0] == '\0')
		return OSAL_ERR_INVALID_PARAM;

	if (device[0] == '/')
		len = osal_snprintf(path, path_size, "%s", device);
	else
		len = osal_snprintf(path, path_size, "/dev/%s", device);

	if (len <= 0 || len >= path_size)
		return OSAL_ERR_INVALID_PARAM;

	return OSAL_SUCCESS;
}

static const char *hal_serial_tty_name_from_path(const char *path)
{
	const char *name;

	if (!path)
		return NULL;

	name = strrchr(path, '/');
	return name ? name + 1 : path;
}

static int32_t hal_serial_open_tty(const char *path, struct tty_struct **tty)
{
	const char *name;
	dev_t devt;
	int ret;

	if (!path || !tty)
		return OSAL_ERR_INVALID_PARAM;

	*tty = NULL;
	name = hal_serial_tty_name_from_path(path);
	if (!name || name[0] == '\0')
		return OSAL_ERR_INVALID_PARAM;

	ret = tty_dev_name_to_number(name, &devt);
	if (ret < 0)
		return hal_serial_errno_to_status(ret);

	*tty = tty_kopen_shared(devt);
	if (IS_ERR(*tty)) {
		ret = PTR_ERR(*tty);
		*tty = NULL;
		return hal_serial_errno_to_status(ret);
	}

	return OSAL_SUCCESS;
}

static void hal_serial_make_raw(struct ktermios *termios)
{
	termios->c_iflag &=
		~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL |
		  IXON | IXOFF | IXANY);
	termios->c_oflag &= ~OPOST;
	termios->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termios->c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB | CRTSCTS);
	termios->c_cflag |= CLOCAL | CREAD;
	termios->c_cc[VMIN] = 0;
	termios->c_cc[VTIME] = 0;
}

static int32_t hal_serial_apply_config(hal_serial_context_t *ctx,
				       const hal_serial_config_t *config)
{
	struct ktermios termios;
	int ret;

	if (!ctx || !ctx->tty || !hal_serial_valid_config(config))
		return OSAL_ERR_INVALID_PARAM;

	termios = ctx->tty->termios;
	hal_serial_make_raw(&termios);

	switch (config->data_bits) {
	case 5:
		termios.c_cflag |= CS5;
		break;
	case 6:
		termios.c_cflag |= CS6;
		break;
	case 7:
		termios.c_cflag |= CS7;
		break;
	case 8:
	default:
		termios.c_cflag |= CS8;
		break;
	}

	if (config->stop_bits == 2)
		termios.c_cflag |= CSTOPB;

	if (config->parity == HAL_SERIAL_PARITY_ODD)
		termios.c_cflag |= PARENB | PARODD;
	else if (config->parity == HAL_SERIAL_PARITY_EVEN)
		termios.c_cflag |= PARENB;

	if (config->flow_control == HAL_SERIAL_FLOW_HW)
		termios.c_cflag |= CRTSCTS;
	else if (config->flow_control == HAL_SERIAL_FLOW_SW)
		termios.c_iflag |= IXON | IXOFF | IXANY;

	tty_termios_encode_baud_rate(&termios, config->baud_rate,
				      config->baud_rate);

	ret = tty_set_termios(ctx->tty, &termios);
	if (ret < 0)
		return hal_serial_errno_to_status(ret);

	ctx->config = *config;
	return OSAL_SUCCESS;
}

static int32_t hal_serial_wait_result(ssize_t ret)
{
	if (ret >= 0)
		return (int32_t)ret;

	return hal_serial_errno_to_status((int)ret);
}

static int32_t hal_serial_begin_op(hal_serial_context_t *ctx)
{
	int32_t ret = OSAL_SUCCESS;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->state_lock);
	if (!ctx->initialized || ctx->closing)
		ret = OSAL_ERR_INVALID_ID;
	else
		ctx->active_ops++;
	osal_mutex_unlock(&ctx->state_lock);

	return ret;
}

static void hal_serial_end_op(hal_serial_context_t *ctx)
{
	bool idle;

	if (!ctx)
		return;

	osal_mutex_lock(&ctx->state_lock);
	if (ctx->active_ops > 0)
		ctx->active_ops--;
	idle = ctx->active_ops == 0;
	osal_mutex_unlock(&ctx->state_lock);

	if (idle)
		wake_up(&ctx->idle_wait);
}

static bool hal_serial_is_closing(hal_serial_context_t *ctx)
{
	bool closing;

	osal_mutex_lock(&ctx->state_lock);
	closing = ctx->closing || !ctx->initialized;
	osal_mutex_unlock(&ctx->state_lock);

	return closing;
}

static bool hal_serial_timeout_expired(unsigned long deadline)
{
	return deadline != MAX_JIFFY_OFFSET && time_after_eq(jiffies, deadline);
}

static unsigned int hal_serial_sleep_ms(unsigned long deadline)
{
	unsigned long remaining;

	if (deadline == MAX_JIFFY_OFFSET)
		return 20;

	remaining = jiffies_to_msecs(deadline - jiffies);
	if (remaining == 0)
		return 1;

	return min_t(unsigned int, remaining, 20);
}

static int32_t hal_serial_rw_loop(hal_serial_context_t *ctx, void *buffer,
				  uint32_t size, int32_t timeout, bool write)
{
	unsigned long deadline;
	loff_t pos = 0;
	ssize_t ret;

	if (!ctx || !buffer || size == 0)
		return OSAL_ERR_INVALID_PARAM;

	if (timeout < 0)
		deadline = MAX_JIFFY_OFFSET;
	else if (timeout == 0)
		deadline = jiffies;
	else
		deadline = jiffies + msecs_to_jiffies(timeout);

	for (;;) {
		if (hal_serial_is_closing(ctx))
			return OSAL_ERR_INVALID_ID;

		if (write)
			ret = kernel_write(ctx->file, buffer, size, &pos);
		else
			ret = kernel_read(ctx->file, buffer, size, &pos);

		if (ret > 0)
			return (int32_t)ret;

		if (ret == 0) {
			if (write || timeout == 0)
				return 0;
		} else if (ret != -EAGAIN && ret != -EWOULDBLOCK &&
		    ret != -ERESTARTSYS)
			return hal_serial_wait_result(ret);

		if (timeout == 0 || hal_serial_timeout_expired(deadline))
			return OSAL_ERR_TIMEOUT;

		msleep(hal_serial_sleep_ms(deadline));
	}
}

int32_t hal_serial_open(const char *device, const hal_serial_config_t *config,
			hal_serial_handle_t *handle)
{
	hal_serial_context_t *ctx;
	int ret;

	if (!device || !config || !handle)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;

	if (!hal_serial_valid_config(config))
		return OSAL_ERR_INVALID_PARAM;

	ctx = osal_zalloc(sizeof(*ctx));
	if (!ctx)
		return OSAL_ERR_NO_MEMORY;

	ret = hal_serial_build_path(device, ctx->path, sizeof(ctx->path));
	if (ret != OSAL_SUCCESS)
		goto err_free;

	ret = osal_mutex_init(&ctx->tx_lock, NULL);
	if (ret != OSAL_SUCCESS)
		goto err_free;

	ret = osal_mutex_init(&ctx->rx_lock, NULL);
	if (ret != OSAL_SUCCESS)
		goto err_tx_mutex;

	ret = osal_mutex_init(&ctx->config_lock, NULL);
	if (ret != OSAL_SUCCESS)
		goto err_rx_mutex;

	ret = osal_mutex_init(&ctx->state_lock, NULL);
	if (ret != OSAL_SUCCESS)
		goto err_config_mutex;

	init_waitqueue_head(&ctx->idle_wait);

	ctx->file = filp_open(ctx->path, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
	if (IS_ERR(ctx->file)) {
		ret = hal_serial_errno_to_status(PTR_ERR(ctx->file));
		ctx->file = NULL;
		goto err_state_mutex;
	}

	ret = hal_serial_open_tty(ctx->path, &ctx->tty);
	if (ret != OSAL_SUCCESS)
		goto err_file;

	ret = hal_serial_apply_config(ctx, config);
	if (ret != OSAL_SUCCESS)
		goto err_tty;

	tty_ldisc_flush(ctx->tty);
	tty_driver_flush_buffer(ctx->tty);

	ctx->initialized = true;
	*handle = ctx;

	LOG_INFO("HAL_SERIAL", "opened %s @ %u bps", ctx->path,
		 config->baud_rate);
	return OSAL_SUCCESS;

err_tty:
	tty_kclose(ctx->tty);
	ctx->tty = NULL;
err_file:
	filp_close(ctx->file, NULL);
	ctx->file = NULL;
err_state_mutex:
	osal_mutex_destroy(&ctx->state_lock);
err_config_mutex:
	osal_mutex_destroy(&ctx->config_lock);
err_rx_mutex:
	osal_mutex_destroy(&ctx->rx_lock);
err_tx_mutex:
	osal_mutex_destroy(&ctx->tx_lock);
err_free:
	osal_free(ctx);
	return ret;
}
EXPORT_SYMBOL_GPL(hal_serial_open);

int32_t hal_serial_close(hal_serial_handle_t handle)
{
	hal_serial_context_t *ctx = handle;
	struct tty_struct *tty;
	struct file *file;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->state_lock);
	ctx->initialized = false;
	ctx->closing = true;
	osal_mutex_unlock(&ctx->state_lock);

	wait_event(ctx->idle_wait, READ_ONCE(ctx->active_ops) == 0);

	osal_mutex_lock(&ctx->config_lock);
	tty = ctx->tty;
	file = ctx->file;
	ctx->tty = NULL;
	ctx->file = NULL;
	osal_mutex_unlock(&ctx->config_lock);

	if (tty) {
		tty_wait_until_sent(tty, msecs_to_jiffies(1000));
		tty_kclose(tty);
	}

	if (file)
		filp_close(file, NULL);

	osal_mutex_destroy(&ctx->config_lock);
	osal_mutex_destroy(&ctx->state_lock);
	osal_mutex_destroy(&ctx->rx_lock);
	osal_mutex_destroy(&ctx->tx_lock);
	osal_free(ctx);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_serial_close);

int32_t hal_serial_write(hal_serial_handle_t handle, const void *buffer,
			 uint32_t size, int32_t timeout)
{
	hal_serial_context_t *ctx = handle;
	int32_t ret;

	if (!ctx || !buffer || size == 0)
		return OSAL_ERR_INVALID_PARAM;

	ret = hal_serial_begin_op(ctx);
	if (ret != OSAL_SUCCESS)
		return ret;

	osal_mutex_lock(&ctx->tx_lock);
	if (!READ_ONCE(ctx->initialized) || !ctx->file) {
		osal_mutex_unlock(&ctx->tx_lock);
		hal_serial_end_op(ctx);
		return OSAL_ERR_INVALID_ID;
	}

	ret = hal_serial_rw_loop(ctx, (void *)buffer, size, timeout, true);
	osal_mutex_unlock(&ctx->tx_lock);
	hal_serial_end_op(ctx);
	return ret;
}
EXPORT_SYMBOL_GPL(hal_serial_write);

int32_t hal_serial_read(hal_serial_handle_t handle, void *buffer,
			uint32_t size, int32_t timeout)
{
	hal_serial_context_t *ctx = handle;
	int32_t ret;

	if (!ctx || !buffer || size == 0)
		return OSAL_ERR_INVALID_PARAM;

	ret = hal_serial_begin_op(ctx);
	if (ret != OSAL_SUCCESS)
		return ret;

	osal_mutex_lock(&ctx->rx_lock);
	if (!READ_ONCE(ctx->initialized) || !ctx->file) {
		osal_mutex_unlock(&ctx->rx_lock);
		hal_serial_end_op(ctx);
		return OSAL_ERR_INVALID_ID;
	}

	ret = hal_serial_rw_loop(ctx, buffer, size, timeout, false);
	osal_mutex_unlock(&ctx->rx_lock);
	hal_serial_end_op(ctx);
	return ret;
}
EXPORT_SYMBOL_GPL(hal_serial_read);

int32_t hal_serial_flush(hal_serial_handle_t handle)
{
	hal_serial_context_t *ctx = handle;
	int32_t ret;

	if (!ctx)
		return OSAL_ERR_INVALID_PARAM;

	ret = hal_serial_begin_op(ctx);
	if (ret != OSAL_SUCCESS)
		return ret;

	osal_mutex_lock(&ctx->config_lock);
	if (!READ_ONCE(ctx->initialized) || !ctx->tty) {
		osal_mutex_unlock(&ctx->config_lock);
		hal_serial_end_op(ctx);
		return OSAL_ERR_INVALID_ID;
	}

	tty_ldisc_flush(ctx->tty);
	tty_driver_flush_buffer(ctx->tty);
	osal_mutex_unlock(&ctx->config_lock);
	hal_serial_end_op(ctx);

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(hal_serial_flush);

int32_t hal_serial_set_config(hal_serial_handle_t handle,
			      const hal_serial_config_t *config)
{
	hal_serial_context_t *ctx = handle;
	int32_t ret;

	if (!ctx || !config)
		return OSAL_ERR_INVALID_PARAM;

	ret = hal_serial_begin_op(ctx);
	if (ret != OSAL_SUCCESS)
		return ret;

	osal_mutex_lock(&ctx->config_lock);
	if (!READ_ONCE(ctx->initialized) || !ctx->tty) {
		osal_mutex_unlock(&ctx->config_lock);
		hal_serial_end_op(ctx);
		return OSAL_ERR_INVALID_ID;
	}

	ret = hal_serial_apply_config(ctx, config);
	osal_mutex_unlock(&ctx->config_lock);
	hal_serial_end_op(ctx);
	return ret;
}
EXPORT_SYMBOL_GPL(hal_serial_set_config);
