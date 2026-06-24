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

static int pdm_mcu_i2c_xfer(struct pdm_mcu_instance *inst, const u8 *tx,
			    u32 tx_len, u8 *rx, u32 rx_len);
static int pdm_mcu_i2c_setup(struct pdm_mcu_instance *inst);
static void pdm_mcu_i2c_cleanup(struct pdm_mcu_instance *inst);
static int pdm_mcu_i2c_probe(struct i2c_client *client);
static void pdm_mcu_i2c_remove(struct i2c_client *client);

static const struct pdm_mcu_transport_ops pdm_mcu_i2c_ops = {
	.type = PDM_MCU_BACKEND_I2C,
	.name = "i2c",
	.capability = PDM_CTL_DEVICE_CAP_TRANSPORT_I2C,
	.max_tx_size = PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_MAX_PREFIX_BYTES,
	.max_rx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.flags = PDM_MCU_TRANSPORT_F_REGISTER_BUS,
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
	LOG_INFO("Bound I2C transport to %s addr=0x%02x",
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

static void pdm_mcu_i2c_remove(struct i2c_client *client)
{
	struct pdm_mcu_native_device *native = i2c_get_clientdata(client);

	pdm_mcu_unregister_native_device(native);
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
		     &pdm_mcu_i2c_ops, pdm_mcu_i2c_driver_register, pdm_mcu_i2c_driver_unregister);
