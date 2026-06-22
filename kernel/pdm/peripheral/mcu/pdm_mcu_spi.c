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

#include "pdm/compat/pdm_compat_features.h"
#include "pdm/core/pdm_backend.h"
#include "pdm_mcu_internal.h"
#include "osal.h"

#if IS_ENABLED(CONFIG_PDM_MCU_SPI) && IS_ENABLED(CONFIG_SPI)
static u8 pdm_mcu_spi_prefix_bytes(struct device_node *np,
				   const char *plain_name,
				   const char *pdm_name,
				   u8 default_value)
{
	u32 value;

	if (!np)
		return default_value;
	if (!of_property_read_u32(np, pdm_name, &value) ||
	    !of_property_read_u32(np, plain_name, &value))
		return min_t(u32, value, PDM_MCU_MAX_PREFIX_BYTES);
	return default_value;
}

static void pdm_mcu_spi_encode_prefix(u8 *buf, u32 value, u8 bytes)
{
	u8 i;

	for (i = 0; i < bytes; i++)
		buf[i] = (u8)(value >> (8U * (bytes - i - 1U)));
}

static int pdm_mcu_spi_xfer(struct pdm_mcu_instance *inst,
			    const u8 *tx, u32 tx_len, u8 *rx, u32 rx_len)
{
	struct spi_device *spi = inst->transport.spi.spi;
	struct spi_transfer xfers[2] = { };
	int xfer_count = 0;

	if (!spi)
		return -ENODEV;
	if (!tx_len && !rx_len)
		return 0;

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
	struct pdm_mcu_native_device *native = inst->pdm_dev->config_data;
	struct device_node *np = inst->pdm_dev->dev.of_node;
	u32 value;

	if (!native || native->type != PDM_MCU_BACKEND_SPI || !native->bus.spi)
		return -ENODEV;

	inst->transport.spi.spi = native->bus.spi;
	inst->transport.spi.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	inst->transport.spi.command_bytes =
		pdm_mcu_spi_prefix_bytes(np, "command-bytes",
					 "pdm,command-bytes", 1U);
	inst->transport.spi.address_bytes =
		pdm_mcu_spi_prefix_bytes(np, "address-bytes",
					 "pdm,address-bytes", 1U);
	if (np && !of_property_read_u32(np, "rx-timeout-ms", &value))
		inst->transport.spi.rx_timeout_ms = value;

	native->inst = inst;
	LOG_INFO("PDM-MCU-SPI", "Bound SPI transport to %s",
		 dev_name(&native->bus.spi->dev));
	return 0;
}

static void pdm_mcu_spi_cleanup(struct pdm_mcu_instance *inst)
{
	struct pdm_mcu_native_device *native = inst->pdm_dev->config_data;

	if (native && native->inst == inst)
		native->inst = NULL;
	inst->transport.spi.spi = NULL;
}

static int pdm_mcu_spi_reset(struct pdm_mcu_instance *inst)
{
	(void)inst;
	return 0;
}

static int pdm_mcu_spi_command(struct pdm_mcu_instance *inst,
			       struct pdm_mcu_command *command)
{
	u8 tx[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_MAX_PREFIX_BYTES];
	u32 tx_len;
	u8 prefix;
	int ret;

	if (command->tx_len > PDM_MCU_MAX_TRANSFER_SIZE ||
	    command->rx_len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	prefix = inst->transport.spi.command_bytes;
	if (command->tx_len + prefix > sizeof(tx))
		return -EMSGSIZE;

	if (prefix)
		pdm_mcu_spi_encode_prefix(tx, command->command, prefix);
	if (command->tx_len)
		memcpy(tx + prefix, command->tx_data, command->tx_len);
	tx_len = prefix + command->tx_len;

	ret = pdm_mcu_spi_xfer(inst, tx, tx_len,
				 command->rx_data, command->rx_len);
	if (ret)
		return ret;
	return 0;
}

static int pdm_mcu_spi_read_data(struct pdm_mcu_instance *inst,
				 struct pdm_mcu_data *data)
{
	u8 tx[PDM_MCU_MAX_PREFIX_BYTES];
	u8 prefix;
	int ret;

	if (data->len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	prefix = inst->transport.spi.address_bytes;
	if (prefix)
		pdm_mcu_spi_encode_prefix(tx, data->address, prefix);

	ret = pdm_mcu_spi_xfer(inst, tx, prefix, data->data, data->len);
	if (ret)
		return ret;
	return 0;
}

static int pdm_mcu_spi_write_data(struct pdm_mcu_instance *inst,
				  const struct pdm_mcu_data *data)
{
	u8 tx[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_MAX_PREFIX_BYTES];
	u8 prefix;

	if (data->len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	prefix = inst->transport.spi.address_bytes;
	if (data->len + prefix > sizeof(tx))
		return -EMSGSIZE;
	if (prefix)
		pdm_mcu_spi_encode_prefix(tx, data->address, prefix);
	if (data->len)
		memcpy(tx + prefix, data->data, data->len);

	return pdm_mcu_spi_xfer(inst, tx, prefix + data->len, NULL, 0);
}

const struct pdm_mcu_transport_ops pdm_mcu_spi_ops = {
	.type = PDM_MCU_BACKEND_SPI,
	.name = "spi",
	.capability = PDM_CTL_DEVICE_CAP_TRANSPORT_SPI,
	.setup = pdm_mcu_spi_setup,
	.cleanup = pdm_mcu_spi_cleanup,
	.reset = pdm_mcu_spi_reset,
	.command = pdm_mcu_spi_command,
	.read_data = pdm_mcu_spi_read_data,
	.write_data = pdm_mcu_spi_write_data,
};

static int pdm_mcu_spi_probe(struct spi_device *spi)
{
	struct pdm_mcu_native_device *native;

	native = devm_kzalloc(&spi->dev, sizeof(*native), GFP_KERNEL);
	if (!native)
		return -ENOMEM;

	native->bus.spi = spi;
	spi_set_drvdata(spi, native);
	return pdm_mcu_register_native_device(&spi->dev,
					      PDM_MCU_BACKEND_SPI, native);
}

#if PDM_KERNEL_HAS_VOID_SPI_REMOVE
static void pdm_mcu_spi_remove(struct spi_device *spi)
#else
static int pdm_mcu_spi_remove(struct spi_device *spi)
#endif
{
	struct pdm_mcu_native_device *native = spi_get_drvdata(spi);

	pdm_mcu_unregister_native_device(native);
	spi_set_drvdata(spi, NULL);
#if !PDM_KERNEL_HAS_VOID_SPI_REMOVE
	return 0;
#endif
}

static const struct of_device_id pdm_mcu_spi_of_match[] = {
	{ .compatible = "pdm,mcu-spi" },
	{ .compatible = "vendor,pdm-mcu-spi" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_spi_of_match);

static const struct spi_device_id pdm_mcu_spi_id[] = {
	{ "pdm-mcu-spi", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, pdm_mcu_spi_id);

static struct spi_driver pdm_mcu_spi_driver = {
	.driver = {
		.name = "pdm-mcu-spi",
		.of_match_table = pdm_mcu_spi_of_match,
	},
	.probe = pdm_mcu_spi_probe,
	.remove = pdm_mcu_spi_remove,
	.id_table = pdm_mcu_spi_id,
};

int pdm_mcu_spi_driver_register(void)
{
	return spi_register_driver(&pdm_mcu_spi_driver);
}

void pdm_mcu_spi_driver_unregister(void)
{
	spi_unregister_driver(&pdm_mcu_spi_driver);
}

pdm_backend_register(mcu_spi, PDM_CTL_DEVICE_TYPE_MCU,
		     PDM_BACKEND_CLASS_TRANSPORT, pdm_mcu_spi_of_match,
		     &pdm_mcu_spi_ops, pdm_mcu_spi_driver_register, pdm_mcu_spi_driver_unregister);
#endif
