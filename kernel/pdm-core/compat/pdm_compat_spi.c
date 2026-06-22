// SPDX-License-Identifier: GPL-2.0

#include <linux/device/bus.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

#include "lpf/compat/lpf_compat_errno.h"
#include "lpf/compat/lpf_compat_spi.h"

static struct spi_device *lpf_compat_spi_find_device(const char *name)
{
	struct device *dev;

	dev = bus_find_device_by_name(&spi_bus_type, NULL, name);
	if (!dev)
		return NULL;

	return to_spi_device(dev);
}

static int32_t lpf_compat_spi_apply_config(struct spi_device *spi,
					   const lpf_spi_config_t *config)
{
	int ret;

	if (!spi || !config)
		return OSAL_ERR_INVALID_PARAM;

	spi->mode = config->mode;
	spi->bits_per_word = config->bits_per_word ? config->bits_per_word : 8;
	spi->max_speed_hz = config->max_speed_hz;

	ret = spi_setup(spi);
	return lpf_compat_errno_to_status(ret);
}

int32_t lpf_compat_spi_open(const lpf_spi_config_t *config,
			    lpf_spi_handle_t *handle)
{
	struct spi_device *spi;
	const char *name;
	const char *slash;
	int32_t ret;

	if (!config || !handle || !config->device)
		return OSAL_ERR_INVALID_PARAM;

	*handle = NULL;
	name = config->device;
	slash = strrchr(config->device, '/');
	if (slash)
		name = slash + 1;

	spi = lpf_compat_spi_find_device(name);
	if (!spi)
		return OSAL_ENOENT;

	ret = lpf_compat_spi_apply_config(spi, config);
	if (ret != OSAL_SUCCESS) {
		spi_dev_put(spi);
		return ret;
	}

	*handle = spi;
	return OSAL_SUCCESS;
}

void lpf_compat_spi_close(lpf_spi_handle_t handle)
{
	if (handle)
		spi_dev_put((struct spi_device *)handle);
}

int32_t lpf_compat_spi_transfer(lpf_spi_handle_t handle,
				const lpf_spi_transfer_t *transfers,
				uint32_t num)
{
	struct spi_device *spi = handle;
	struct spi_transfer *xfers;
	struct spi_message message;
	uint32_t i;
	int ret;

	if (!spi || !transfers || num == 0)
		return OSAL_ERR_INVALID_PARAM;

	xfers = osal_zalloc(sizeof(*xfers) * num);
	if (!xfers)
		return OSAL_ERR_NO_MEMORY;

	spi_message_init(&message);
	for (i = 0; i < num; i++) {
		if ((!transfers[i].tx_buf && !transfers[i].rx_buf) ||
		    transfers[i].len == 0) {
			osal_free(xfers);
			return OSAL_ERR_INVALID_PARAM;
		}

		xfers[i].tx_buf = transfers[i].tx_buf;
		xfers[i].rx_buf = transfers[i].rx_buf;
		xfers[i].len = transfers[i].len;
		xfers[i].speed_hz = transfers[i].speed_hz;
		xfers[i].bits_per_word = transfers[i].bits_per_word;
		xfers[i].delay.value = transfers[i].delay_usecs;
		xfers[i].delay.unit = SPI_DELAY_UNIT_USECS;
		xfers[i].cs_change = transfers[i].cs_change ? 1 : 0;
		spi_message_add_tail(&xfers[i], &message);
	}

	ret = spi_sync(spi, &message);
	osal_free(xfers);

	return lpf_compat_errno_to_status(ret);
}

int32_t lpf_compat_spi_set_config(lpf_spi_handle_t handle,
				  const lpf_spi_config_t *config)
{
	return lpf_compat_spi_apply_config((struct spi_device *)handle, config);
}
