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

#include "pdm/compat/pdm_compat_features.h"
#include "pdm_mcu_internal.h"
#include "osal.h"

#if IS_ENABLED(CONFIG_PDM_MCU_I2C) && IS_ENABLED(CONFIG_I2C)
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

static void pdm_mcu_i2c_encode_prefix(u8 *buf, u32 value, u8 bytes)
{
	u8 i;

	for (i = 0; i < bytes; i++)
		buf[i] = (u8)(value >> (8U * (bytes - i - 1U)));
}

static int pdm_mcu_i2c_xfer(struct pdm_mcu_instance *inst,
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
	struct pdm_mcu_native_device *native = inst->pdm_dev->config_data;
	struct device_node *np = inst->pdm_dev->dev.of_node;
	u32 value;

	if (!native || native->type != PDM_MCU_BACKEND_I2C || !native->bus.i2c)
		return -ENODEV;

	inst->transport.i2c.client = native->bus.i2c;
	inst->transport.i2c.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	inst->transport.i2c.command_bytes =
		pdm_mcu_i2c_prefix_bytes(np, "command-bytes",
					 "pdm,command-bytes", 1U);
	inst->transport.i2c.address_bytes =
		pdm_mcu_i2c_prefix_bytes(np, "address-bytes",
					 "pdm,address-bytes", 1U);
	if (np && !of_property_read_u32(np, "rx-timeout-ms", &value))
		inst->transport.i2c.rx_timeout_ms = value;

	native->inst = inst;
	LOG_INFO("PDM-MCU-I2C", "Bound I2C transport to %s addr=0x%02x",
		 dev_name(&native->bus.i2c->dev), native->bus.i2c->addr);
	return 0;
}

static void pdm_mcu_i2c_cleanup(struct pdm_mcu_instance *inst)
{
	struct pdm_mcu_native_device *native = inst->pdm_dev->config_data;

	if (native && native->inst == inst)
		native->inst = NULL;
	inst->transport.i2c.client = NULL;
}

static int pdm_mcu_i2c_reset(struct pdm_mcu_instance *inst)
{
	(void)inst;
	return 0;
}

static int pdm_mcu_i2c_command(struct pdm_mcu_instance *inst,
			       struct pdm_mcu_command *command)
{
	u8 tx[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_MAX_PREFIX_BYTES];
	u32 tx_len;
	u8 prefix;
	int ret;

	if (command->tx_len > PDM_MCU_MAX_TRANSFER_SIZE ||
	    command->rx_len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	prefix = inst->transport.i2c.command_bytes;
	if (command->tx_len + prefix > sizeof(tx))
		return -EMSGSIZE;

	if (prefix)
		pdm_mcu_i2c_encode_prefix(tx, command->command, prefix);
	if (command->tx_len)
		memcpy(tx + prefix, command->tx_data, command->tx_len);
	tx_len = prefix + command->tx_len;

	ret = pdm_mcu_i2c_xfer(inst, tx, tx_len,
				 command->rx_data, command->rx_len);
	if (ret)
		return ret;
	return 0;
}

static int pdm_mcu_i2c_read_data(struct pdm_mcu_instance *inst,
				 struct pdm_mcu_data *data)
{
	u8 tx[PDM_MCU_MAX_PREFIX_BYTES];
	u8 prefix;
	int ret;

	if (data->len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	prefix = inst->transport.i2c.address_bytes;
	if (prefix)
		pdm_mcu_i2c_encode_prefix(tx, data->address, prefix);

	ret = pdm_mcu_i2c_xfer(inst, tx, prefix, data->data, data->len);
	if (ret)
		return ret;
	return 0;
}

static int pdm_mcu_i2c_write_data(struct pdm_mcu_instance *inst,
				  const struct pdm_mcu_data *data)
{
	u8 tx[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_MAX_PREFIX_BYTES];
	u8 prefix;

	if (data->len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	prefix = inst->transport.i2c.address_bytes;
	if (data->len + prefix > sizeof(tx))
		return -EMSGSIZE;
	if (prefix)
		pdm_mcu_i2c_encode_prefix(tx, data->address, prefix);
	if (data->len)
		memcpy(tx + prefix, data->data, data->len);

	return pdm_mcu_i2c_xfer(inst, tx, prefix + data->len, NULL, 0);
}

const struct pdm_mcu_transport_ops pdm_mcu_i2c_ops = {
	.type = PDM_MCU_BACKEND_I2C,
	.name = "i2c",
	.capability = PDM_CTL_DEVICE_CAP_TRANSPORT_I2C,
	.setup = pdm_mcu_i2c_setup,
	.cleanup = pdm_mcu_i2c_cleanup,
	.reset = pdm_mcu_i2c_reset,
	.command = pdm_mcu_i2c_command,
	.read_data = pdm_mcu_i2c_read_data,
	.write_data = pdm_mcu_i2c_write_data,
};

static int pdm_mcu_i2c_probe(struct i2c_client *client)
{
	struct pdm_mcu_native_device *native;

	native = devm_kzalloc(&client->dev, sizeof(*native), GFP_KERNEL);
	if (!native)
		return -ENOMEM;

	native->bus.i2c = client;
	i2c_set_clientdata(client, native);
	return pdm_mcu_register_native_device(&client->dev,
					      PDM_MCU_BACKEND_I2C, native);
}

#if PDM_KERNEL_HAS_VOID_I2C_REMOVE
static void pdm_mcu_i2c_remove(struct i2c_client *client)
#else
static int pdm_mcu_i2c_remove(struct i2c_client *client)
#endif
{
	struct pdm_mcu_native_device *native = i2c_get_clientdata(client);

	pdm_mcu_unregister_native_device(native);
	i2c_set_clientdata(client, NULL);
#if !PDM_KERNEL_HAS_VOID_I2C_REMOVE
	return 0;
#endif
}

static const struct of_device_id pdm_mcu_i2c_of_match[] = {
	{ .compatible = "pdm,mcu-i2c" },
	{ .compatible = "vendor,pdm-mcu-i2c" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_i2c_of_match);

static const struct i2c_device_id pdm_mcu_i2c_id[] = {
	{ "pdm-mcu-i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pdm_mcu_i2c_id);

static struct i2c_driver pdm_mcu_i2c_driver = {
	.driver = {
		.name = "pdm-mcu-i2c",
		.of_match_table = pdm_mcu_i2c_of_match,
	},
#if PDM_KERNEL_HAS_I2C_PROBE_ONE_ARG
	.probe = pdm_mcu_i2c_probe,
#else
	.probe_new = pdm_mcu_i2c_probe,
#endif
	.remove = pdm_mcu_i2c_remove,
	.id_table = pdm_mcu_i2c_id,
};

int pdm_mcu_i2c_driver_register(void)
{
	return i2c_register_driver(THIS_MODULE, &pdm_mcu_i2c_driver);
}

void pdm_mcu_i2c_driver_unregister(void)
{
	i2c_del_driver(&pdm_mcu_i2c_driver);
}
#endif
