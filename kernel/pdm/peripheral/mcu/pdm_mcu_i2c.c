// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_i2c.c
 * @brief I2C transport for PDM MCU devices
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "pdm/compat/pdm_compat_i2c.h"
#include "pdm/core/driver/pdm_backend.h"
#include "pdm_mcu_internal.h"
#include "osal.h"

static const struct of_device_id pdm_mcu_i2c_of_match[] = {
	{ .compatible = "pdm,mcu-i2c" },
	{ .compatible = "vendor,pdm-mcu-i2c" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_i2c_of_match);

static const struct i2c_device_id pdm_mcu_i2c_id[] = {
	{ "mcu-i2c", 0 },
	{ "pdm-mcu-i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pdm_mcu_i2c_id);

static int pdm_mcu_i2c_setup(struct pdm_mcu_instance *inst);
static void pdm_mcu_i2c_cleanup(struct pdm_mcu_instance *inst);
static int pdm_mcu_i2c_xfer(struct pdm_mcu_instance *inst,
			    struct pdm_mcu_xfer *xfer);
static int pdm_mcu_i2c_cmd_xfer(struct pdm_mcu_instance *inst, u32 command,
				const u8 *tx, u32 tx_len, u8 *rx, u32 *rx_len);
static int pdm_mcu_i2c_data_read(struct pdm_mcu_instance *inst,
				 u32 *address, u8 *buf, u32 *len);
static int pdm_mcu_i2c_data_write(struct pdm_mcu_instance *inst, u32 address,
				  const u8 *buf, u32 len);
static int pdm_mcu_i2c_probe(struct i2c_client *client);
static void pdm_mcu_i2c_remove(struct i2c_client *client);

static const struct pdm_mcu_transport_ops pdm_mcu_i2c_ops = {
	.name = "i2c",
	.capability = PDM_CTL_DEVICE_CAP_TRANSPORT_I2C,
	.max_tx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.max_rx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.setup = pdm_mcu_i2c_setup,
	.cleanup = pdm_mcu_i2c_cleanup,
	.xfer = pdm_mcu_i2c_xfer,
};

static struct pdm_compat_i2c_driver pdm_mcu_i2c_driver = {
	.driver = {
		.driver = {
			.name = "pdm-mcu-i2c",
			.of_match_table = pdm_mcu_i2c_of_match,
		},
		.id_table = pdm_mcu_i2c_id,
	},
	.probe = pdm_mcu_i2c_probe,
	.remove = pdm_mcu_i2c_remove,
};

static void pdm_mcu_i2c_encode_prefix(u8 *buf, u32 value, u8 bytes)
{
	u8 i;

	for (i = 0; i < bytes; i++)
		buf[i] = (u8)(value >> (8U * (bytes - i - 1U)));
}

static u8 pdm_mcu_i2c_prefix_bytes(struct device_node *np,
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

static int pdm_mcu_i2c_bus_xfer(struct pdm_mcu_instance *inst,
				const u8 *tx, u32 tx_len, u8 *rx, u32 rx_len)
{
	struct i2c_client *client = inst->transport.i2c.client;
	struct i2c_msg msgs[2];
	int msg_count = 0;
	int ret;

	if (!client)
		return -ENODEV;
	if (!tx_len && !rx_len)
		return 0;

	if (tx_len) {
		msgs[msg_count].addr = client->addr;
		msgs[msg_count].flags = 0;
		msgs[msg_count].len = tx_len;
		msgs[msg_count].buf = (u8 *)tx;
		msg_count++;
	}
	if (rx_len) {
		msgs[msg_count].addr = client->addr;
		msgs[msg_count].flags = I2C_M_RD;
		msgs[msg_count].len = rx_len;
		msgs[msg_count].buf = rx;
		msg_count++;
	}

	ret = i2c_transfer(client->adapter, msgs, msg_count);
	if (ret < 0)
		return ret;
	return ret == msg_count ? 0 : -EIO;
}

static int pdm_mcu_i2c_setup(struct pdm_mcu_instance *inst)
{
	struct pdm_mcu_bus_device *bus_dev = inst->pdm_dev->config_data;
	struct device_node *np = inst->pdm_dev->dev.of_node;
	u32 value;

	if (!bus_dev || bus_dev->type != PDM_MCU_BACKEND_I2C || !bus_dev->bus.i2c)
		return -ENODEV;

	inst->transport.i2c.client = bus_dev->bus.i2c;
	inst->transport.i2c.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	inst->transport.i2c.command_bytes =
		pdm_mcu_i2c_prefix_bytes(np, "command-bytes",
					 "pdm,command-bytes", 1U);
	inst->transport.i2c.address_bytes =
		pdm_mcu_i2c_prefix_bytes(np, "address-bytes",
					 "pdm,address-bytes", 1U);
	if (np && !of_property_read_u32(np, "rx-timeout-ms", &value))
		inst->transport.i2c.rx_timeout_ms = value;

	bus_dev->inst = inst;
	LOG_INFO("Bound I2C transport to %s addr=0x%02x",
		 dev_name(&bus_dev->bus.i2c->dev), bus_dev->bus.i2c->addr);
	return 0;
}

static void pdm_mcu_i2c_cleanup(struct pdm_mcu_instance *inst)
{
	struct pdm_mcu_bus_device *bus_dev = inst->pdm_dev->config_data;

	if (bus_dev && bus_dev->inst == inst)
		bus_dev->inst = NULL;
	inst->transport.i2c.client = NULL;
}

static int pdm_mcu_i2c_cmd_xfer(struct pdm_mcu_instance *inst, u32 command,
				const u8 *tx, u32 tx_len, u8 *rx, u32 *rx_len)
{
	u8 buf[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_MAX_PREFIX_BYTES];
	u8 prefix = inst->transport.i2c.command_bytes;
	u32 expect = rx_len ? *rx_len : 0;
	int ret;

	if (!prefix)
		return -EOPNOTSUPP;
	if (tx_len + prefix > sizeof(buf))
		return -EMSGSIZE;
	if (expect > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	pdm_mcu_i2c_encode_prefix(buf, command, prefix);
	if (tx_len)
		memcpy(buf + prefix, tx, tx_len);

	ret = pdm_mcu_i2c_bus_xfer(inst, buf, tx_len + prefix, rx, expect);
	if (ret)
		return ret;
	if (rx_len)
		*rx_len = expect;
	return 0;
}

static int pdm_mcu_i2c_data_read(struct pdm_mcu_instance *inst,
				 u32 *address, u8 *buf, u32 *len)
{
	u8 tx[PDM_MCU_MAX_PREFIX_BYTES];
	u8 prefix = inst->transport.i2c.address_bytes;

	if (*len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;
	if (prefix)
		pdm_mcu_i2c_encode_prefix(tx, *address, prefix);

	return pdm_mcu_i2c_bus_xfer(inst, tx, prefix, buf, *len);
}

static int pdm_mcu_i2c_data_write(struct pdm_mcu_instance *inst, u32 address,
				  const u8 *buf, u32 len)
{
	u8 tx[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_MAX_PREFIX_BYTES];
	u8 prefix = inst->transport.i2c.address_bytes;

	if (len + prefix > sizeof(tx))
		return -EMSGSIZE;
	if (prefix)
		pdm_mcu_i2c_encode_prefix(tx, address, prefix);
	if (len)
		memcpy(tx + prefix, buf, len);

	return pdm_mcu_i2c_bus_xfer(inst, tx, prefix + len, NULL, 0);
}

static int pdm_mcu_i2c_xfer(struct pdm_mcu_instance *inst,
			    struct pdm_mcu_xfer *xfer)
{
	u32 len;
	int ret;

	switch (xfer->type) {
	case PDM_MCU_XFER_CMD:
		len = xfer->rx_len;
		ret = pdm_mcu_i2c_cmd_xfer(inst, xfer->id, xfer->tx,
					   xfer->tx_len, xfer->rx, &len);
		if (ret)
			return ret;
		xfer->actual_rx_len = len;
		return 0;
	case PDM_MCU_XFER_DATA_READ:
		len = xfer->rx_len;
		ret = pdm_mcu_i2c_data_read(inst, &xfer->id, xfer->rx, &len);
		if (ret)
			return ret;
		xfer->actual_rx_len = len;
		return 0;
	case PDM_MCU_XFER_DATA_WRITE:
		return pdm_mcu_i2c_data_write(inst, xfer->id, xfer->tx,
					      xfer->tx_len);
	default:
		return -EINVAL;
	}
}

static int pdm_mcu_i2c_probe(struct i2c_client *client)
{
	struct pdm_mcu_bus_device *bus_dev;

	bus_dev = devm_kzalloc(&client->dev, sizeof(*bus_dev), GFP_KERNEL);
	if (!bus_dev)
		return -ENOMEM;

	bus_dev->bus.i2c = client;
	i2c_set_clientdata(client, bus_dev);
	return pdm_mcu_register_bus_device(&client->dev,
					      PDM_MCU_BACKEND_I2C, bus_dev);
}

static void pdm_mcu_i2c_remove(struct i2c_client *client)
{
	struct pdm_mcu_bus_device *bus_dev = i2c_get_clientdata(client);

	pdm_mcu_unregister_bus_device(bus_dev);
	i2c_set_clientdata(client, NULL);
}

static int pdm_mcu_i2c_driver_register(void)
{
	return pdm_compat_i2c_driver_register(&pdm_mcu_i2c_driver);
}

static void pdm_mcu_i2c_driver_unregister(void)
{
	pdm_compat_i2c_driver_unregister(&pdm_mcu_i2c_driver);
}

pdm_backend_register(mcu_i2c, PDM_CTL_DEVICE_TYPE_MCU,
		     PDM_BACKEND_CLASS_TRANSPORT, pdm_mcu_i2c_of_match,
		     &pdm_mcu_i2c_ops, pdm_mcu_i2c_driver_register,
		     pdm_mcu_i2c_driver_unregister);
