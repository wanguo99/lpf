// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_spi.c
 * @brief SPI transport for PDM MCU devices
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/string.h>

#include "pdm/compat/pdm_compat_spi.h"
#include "pdm/registry/pdm_backend.h"
#include "pdm/pdm_mcu.h"
#include "pdm_mcu_internal.h"
#include "osal.h"

static void pdm_mcu_spi_encode_prefix(u8 *buf, u32 value, u8 bytes)
{
	u8 i;

	for (i = 0; i < bytes; i++) {
		buf[i] = (u8)(value >> (8U * (bytes - i - 1U)));
	}
}

static u8 pdm_mcu_spi_prefix_bytes(struct device_node *np,
				   const char *plain_name,
				   const char *pdm_name,
				   u8 default_value)
{
	u32 value;

	if (!np) {
		return default_value;
	}
	if (!of_property_read_u32(np, pdm_name, &value) ||
	    !of_property_read_u32(np, plain_name, &value)) {
		return min_t(u32, value, PDM_MCU_MAX_PREFIX_BYTES);
	}
	return default_value;
}

static int pdm_mcu_spi_bus_xfer(struct pdm_mcu_instance *inst,
				const u8 *tx, u32 tx_len, u8 *rx, u32 rx_len)
{
	struct spi_device *spi = inst->transport.spi.spi;
	struct spi_transfer xfers[2] = { };
	int xfer_count = 0;

	if (!spi) {
		return -ENODEV;
	}
	if (!tx_len && !rx_len) {
		return 0;
	}

	if (tx_len) {
		xfers[xfer_count].tx_buf = tx;
		xfers[xfer_count].len = tx_len;
		xfer_count++;
	}
	if (rx_len) {
		xfers[xfer_count].rx_buf = rx;
		xfers[xfer_count].len = rx_len;
		xfer_count++;
	}

	return spi_sync_transfer(spi, xfers, xfer_count);
}

static int pdm_mcu_spi_setup(struct pdm_mcu_instance *inst)
{
	struct pdm_mcu_bus_device *bus_dev = inst->base.pdm_dev->config_data;
	struct device_node *np = inst->base.pdm_dev->dev.of_node;
	u32 value;

	if (!bus_dev || bus_dev->type != PDM_MCU_BACKEND_SPI || !bus_dev->bus.spi) {
		return -ENODEV;
	}

	inst->transport.spi.spi = bus_dev->bus.spi;
	inst->transport.spi.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	inst->transport.spi.command_bytes =
		pdm_mcu_spi_prefix_bytes(np, "command-bytes",
					 "pdm,command-bytes", 1U);
	if (np && !of_property_read_u32(np, "rx-timeout-ms", &value)) {
		inst->transport.spi.rx_timeout_ms = value;
	}

	bus_dev->inst = inst;
	LOG_INFO("Bound SPI transport to %s",
		 dev_name(&bus_dev->bus.spi->dev));
	return 0;
}

static void pdm_mcu_spi_cleanup(struct pdm_mcu_instance *inst)
{
	struct pdm_mcu_bus_device *bus_dev = inst->base.pdm_dev->config_data;

	if (bus_dev && bus_dev->inst == inst) {
		bus_dev->inst = NULL;
	}
	inst->transport.spi.spi = NULL;
}

static int pdm_mcu_spi_cmd_xfer(struct pdm_mcu_instance *inst, u32 command,
				const u8 *tx, u32 tx_len, u8 *rx, u32 *rx_len)
{
	u8 buf[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_MAX_PREFIX_BYTES];
	u8 prefix = inst->transport.spi.command_bytes;
	u32 expect = rx_len ? *rx_len : 0;
	int ret;

	if (!prefix) {
		return -EOPNOTSUPP;
	}
	if (tx_len + prefix > sizeof(buf)) {
		return -EMSGSIZE;
	}
	if (expect > PDM_MCU_MAX_TRANSFER_SIZE) {
		return -EMSGSIZE;
	}

	pdm_mcu_spi_encode_prefix(buf, command, prefix);
	if (tx_len) {
		memcpy(buf + prefix, tx, tx_len);
	}

	ret = pdm_mcu_spi_bus_xfer(inst, buf, tx_len + prefix, rx, expect);
	if (ret) {
		return ret;
	}
	if (rx_len) {
		*rx_len = expect;
	}
	return 0;
}

static int pdm_mcu_spi_xfer(struct pdm_mcu_instance *inst,
			    struct pdm_mcu_xfer *xfer)
{
	u32 len = xfer->rx_len;
	int ret;

	ret = pdm_mcu_spi_cmd_xfer(inst, xfer->command, xfer->tx,
				   xfer->tx_len, xfer->rx, &len);
	if (ret) {
		return ret;
	}

	xfer->actual_rx_len = len;
	return 0;
}

static int pdm_mcu_spi_probe(struct spi_device *spi)
{
	struct pdm_mcu_bus_device *bus_dev;

	bus_dev = devm_kzalloc(&spi->dev, sizeof(*bus_dev), GFP_KERNEL);
	if (!bus_dev) {
		return -ENOMEM;
	}

	bus_dev->bus.spi = spi;
	spi_set_drvdata(spi, bus_dev);
	return pdm_mcu_register_bus_device(&spi->dev,
					      PDM_MCU_BACKEND_SPI, bus_dev);
}

static void pdm_mcu_spi_remove(struct spi_device *spi)
{
	struct pdm_mcu_bus_device *bus_dev = spi_get_drvdata(spi);

	pdm_mcu_unregister_bus_device(bus_dev);
	spi_set_drvdata(spi, NULL);
}

static const struct of_device_id pdm_mcu_spi_of_match[] = {
	{ .compatible = "pdm,mcu-spi" },
	{ .compatible = "vendor,pdm-mcu-spi" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_spi_of_match);

static const struct spi_device_id pdm_mcu_spi_id[] = {
	{ "mcu-spi", 0 },
	{ "pdm-mcu-spi", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, pdm_mcu_spi_id);

static const struct pdm_mcu_transport_ops pdm_mcu_spi_ops = {
	.name = "spi",
	.capability = PDM_MCU_CAP_TRANSPORT_SPI,
	.max_tx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.max_rx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.setup = pdm_mcu_spi_setup,
	.cleanup = pdm_mcu_spi_cleanup,
	.xfer = pdm_mcu_spi_xfer,
};

static struct pdm_compat_spi_driver pdm_mcu_spi_driver = {
	.driver = {
		.driver = {
			.name = "pdm-mcu-spi",
			.of_match_table = pdm_mcu_spi_of_match,
		},
		.id_table = pdm_mcu_spi_id,
	},
	.probe = pdm_mcu_spi_probe,
	.remove = pdm_mcu_spi_remove,
};

static int pdm_mcu_spi_driver_register(void)
{
	return pdm_compat_spi_driver_register(&pdm_mcu_spi_driver);
}

static void pdm_mcu_spi_driver_unregister(void)
{
	pdm_compat_spi_driver_unregister(&pdm_mcu_spi_driver);
}

pdm_backend_register(mcu_spi, PDM_MCU_DEVICE_TYPE,
		     PDM_BACKEND_CLASS_TRANSPORT, pdm_mcu_spi_of_match,
		     &pdm_mcu_spi_ops, pdm_mcu_spi_driver_register,
		     pdm_mcu_spi_driver_unregister);
