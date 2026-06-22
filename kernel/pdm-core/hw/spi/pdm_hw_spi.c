// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>

#include "osal.h"
#include "lpf/hw/lpf_hw_spi.h"
#include "lpf/soc/lpf_soc_adapter.h"

typedef struct {
	lpf_spi_handle_t spi;
	osal_mutex_t lock;
	lpf_spi_config_t config;
	bool initialized;
} lpf_hw_bus_spi_context_t;

static void lpf_hw_bus_spi_fill_lpf_config(const lpf_spi_config_t *src,
				    lpf_spi_config_t *dst)
{
	dst->device = src->device;
	dst->mode = src->mode;
	dst->bits_per_word = src->bits_per_word;
	dst->max_speed_hz = src->max_speed_hz;
	dst->timeout = src->timeout;
}

static int32_t lpf_hw_bus_spi_apply_config(lpf_hw_bus_spi_context_t *ctx,
				    const lpf_spi_config_t *config)
{
	lpf_spi_config_t lpf_config;
	int32_t ret;

	if (!ctx || !ctx->spi || !config)
		return OSAL_ERR_INVALID_PARAM;

	lpf_hw_bus_spi_fill_lpf_config(config, &lpf_config);
	ret = lpf_soc_spi_set_config(ctx->spi, &lpf_config);
	if (ret != OSAL_SUCCESS)
		return ret;

	ctx->config = *config;
	return OSAL_SUCCESS;
}

int32_t lpf_hw_bus_spi_open(const lpf_spi_config_t *config, lpf_hw_bus_spi_handle_t *handle)
{
	lpf_hw_bus_spi_context_t *ctx;
	lpf_spi_config_t lpf_config;
	int32_t ret;

	if (!config || !handle || !config->device)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;

	ctx = osal_zalloc(sizeof(*ctx));
	if (!ctx)
		return OSAL_ERR_NO_MEMORY;

	ret = osal_mutex_init(&ctx->lock, NULL);
	if (ret != OSAL_SUCCESS)
		goto err_free;

	lpf_hw_bus_spi_fill_lpf_config(config, &lpf_config);
	ret = lpf_soc_spi_open(&lpf_config, &ctx->spi);
	if (ret != OSAL_SUCCESS)
		goto err_mutex;

	ctx->config = *config;
	ctx->initialized = true;
	*handle = ctx;

	LOG_INFO("LPF_HW_BUS_SPI", "opened %s", config->device);
	return OSAL_SUCCESS;

err_mutex:
	osal_mutex_destroy(&ctx->lock);
err_free:
	osal_free(ctx);
	return ret;
}
EXPORT_SYMBOL_GPL(lpf_hw_bus_spi_open);

int32_t lpf_hw_bus_spi_close(lpf_hw_bus_spi_handle_t handle)
{
	lpf_hw_bus_spi_context_t *ctx = handle;
	lpf_spi_handle_t spi;

	if (!handle)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	ctx->initialized = false;
	spi = ctx->spi;
	ctx->spi = NULL;
	osal_mutex_unlock(&ctx->lock);

	if (spi)
		lpf_soc_spi_close(spi);

	osal_mutex_destroy(&ctx->lock);
	osal_free(ctx);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_hw_bus_spi_close);

int32_t lpf_hw_bus_spi_write(lpf_hw_bus_spi_handle_t handle, const uint8_t *buffer,
		      uint32_t size)
{
	if (!handle || !buffer)
		return OSAL_ERR_INVALID_PARAM;

	return lpf_hw_bus_spi_transfer(handle, buffer, NULL, size);
}
EXPORT_SYMBOL_GPL(lpf_hw_bus_spi_write);

int32_t lpf_hw_bus_spi_read(lpf_hw_bus_spi_handle_t handle, uint8_t *buffer, uint32_t size)
{
	if (!handle || !buffer)
		return OSAL_ERR_INVALID_PARAM;

	return lpf_hw_bus_spi_transfer(handle, NULL, buffer, size);
}
EXPORT_SYMBOL_GPL(lpf_hw_bus_spi_read);

int32_t lpf_hw_bus_spi_transfer(lpf_hw_bus_spi_handle_t handle, const uint8_t *tx_buffer,
			 uint8_t *rx_buffer, uint32_t size)
{
	lpf_spi_transfer_t transfer = {
		.tx_buf = tx_buffer,
		.rx_buf = rx_buffer,
		.len = size,
	};

	if (!handle || (!tx_buffer && !rx_buffer) || size == 0)
		return OSAL_ERR_INVALID_PARAM;

	return lpf_hw_bus_spi_transfer_multi(handle, &transfer, 1);
}
EXPORT_SYMBOL_GPL(lpf_hw_bus_spi_transfer);

int32_t lpf_hw_bus_spi_transfer_multi(lpf_hw_bus_spi_handle_t handle,
			       lpf_spi_transfer_t *transfers, uint32_t num)
{
	lpf_hw_bus_spi_context_t *ctx = handle;
	lpf_spi_transfer_t *xfers;
	uint32_t i;
	int32_t ret;

	if (!ctx || !transfers || num == 0)
		return OSAL_ERR_INVALID_PARAM;

	xfers = osal_zalloc(sizeof(*xfers) * num);
	if (!xfers)
		return OSAL_ERR_NO_MEMORY;

	for (i = 0; i < num; i++) {
		if ((!transfers[i].tx_buf && !transfers[i].rx_buf) ||
		    transfers[i].len == 0) {
			osal_free(xfers);
			return OSAL_ERR_INVALID_PARAM;
		}

		xfers[i].tx_buf = transfers[i].tx_buf;
		xfers[i].rx_buf = transfers[i].rx_buf;
		xfers[i].len = transfers[i].len;
		xfers[i].speed_hz = transfers[i].speed_hz ?
					    transfers[i].speed_hz :
					    ctx->config.max_speed_hz;
		xfers[i].bits_per_word = transfers[i].bits_per_word ?
						 transfers[i].bits_per_word :
						 ctx->config.bits_per_word;
		xfers[i].delay_usecs = transfers[i].delay_usecs;
		xfers[i].cs_change = transfers[i].cs_change;
	}

	osal_mutex_lock(&ctx->lock);
	if (!ctx->initialized || !ctx->spi) {
		osal_mutex_unlock(&ctx->lock);
		osal_free(xfers);
		return OSAL_ERR_INVALID_ID;
	}

	ret = lpf_soc_spi_transfer(ctx->spi, xfers, num);
	osal_mutex_unlock(&ctx->lock);
	osal_free(xfers);

	if (ret != OSAL_SUCCESS) {
		LOG_ERROR("LPF_HW_BUS_SPI", "transfer failed: %d", ret);
		return ret;
	}

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(lpf_hw_bus_spi_transfer_multi);

int32_t lpf_hw_bus_spi_set_config(lpf_hw_bus_spi_handle_t handle,
			   const lpf_spi_config_t *config)
{
	lpf_hw_bus_spi_context_t *ctx = handle;
	int ret;

	if (!handle || !config)
		return OSAL_ERR_INVALID_PARAM;

	osal_mutex_lock(&ctx->lock);
	if (!ctx->initialized || !ctx->spi) {
		osal_mutex_unlock(&ctx->lock);
		return OSAL_ERR_INVALID_ID;
	}

	ret = lpf_hw_bus_spi_apply_config(ctx, config);
	osal_mutex_unlock(&ctx->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(lpf_hw_bus_spi_set_config);
