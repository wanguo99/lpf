// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_protocol.c
 * @brief PDM MCU protocol layer between ioctl semantics and transport ops
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>

#include "pdm_mcu_internal.h"

enum pdm_mcu_protocol_cmd {
	PDM_MCU_PROTOCOL_CMD_GET_VERSION = 0x00000001U,
	PDM_MCU_PROTOCOL_CMD_GET_STATUS = 0x00000002U,
	PDM_MCU_PROTOCOL_CMD_RESET = 0x00000003U,
};

static void pdm_mcu_protocol_encode_be32(u8 *buf, u32 value)
{
	buf[0] = (u8)(value >> 24);
	buf[1] = (u8)(value >> 16);
	buf[2] = (u8)(value >> 8);
	buf[3] = (u8)value;
}

static u32 pdm_mcu_protocol_max_tx(const struct pdm_mcu_instance *inst)
{
	if (inst->ops->max_tx_size)
		return min_t(u32, inst->ops->max_tx_size,
			     PDM_MCU_MAX_TRANSFER_SIZE);

	return PDM_MCU_MAX_TRANSFER_SIZE;
}

static u32 pdm_mcu_protocol_max_rx(const struct pdm_mcu_instance *inst)
{
	if (inst->ops->max_rx_size)
		return min_t(u32, inst->ops->max_rx_size,
			     PDM_MCU_MAX_TRANSFER_SIZE);

	return PDM_MCU_MAX_TRANSFER_SIZE;
}

static int pdm_mcu_protocol_xfer(struct pdm_mcu_instance *inst,
				   struct pdm_mcu_xfer *xfer)
{
	int ret;

	if (!inst->ops->xfer)
		return -EOPNOTSUPP;
	if ((xfer->tx_len && !xfer->tx) || (xfer->rx_len && !xfer->rx))
		return -EINVAL;
	if (xfer->tx_len > pdm_mcu_protocol_max_tx(inst) ||
	    xfer->rx_len > pdm_mcu_protocol_max_rx(inst))
		return -EMSGSIZE;

	xfer->actual_rx_len = 0;
	ret = inst->ops->xfer(inst, xfer);
	if (ret)
		return ret;
	if (xfer->actual_rx_len > xfer->rx_len)
		return -EMSGSIZE;

	return 0;
}

static int pdm_mcu_protocol_cmd_xfer(struct pdm_mcu_instance *inst,
				     u32 command, const u8 *payload,
				     u32 payload_len, u8 *response,
				     u32 *response_len)
{
	struct pdm_mcu_xfer xfer = {
		.type = PDM_MCU_XFER_CMD,
		.id = command,
		.tx = payload,
		.tx_len = payload_len,
		.rx = response,
		.rx_len = response_len ? *response_len : 0,
	};
	int ret;

	ret = pdm_mcu_protocol_xfer(inst, &xfer);
	if (ret)
		return ret;
	if (response_len)
		*response_len = xfer.actual_rx_len;
	return 0;
}

static int pdm_mcu_protocol_cmd_expect(struct pdm_mcu_instance *inst,
				       enum pdm_mcu_protocol_cmd command,
				       const u8 *payload, u32 payload_len,
				       u8 *response, u32 response_len)
{
	u32 rx_len = response_len;
	int ret;

	ret = pdm_mcu_protocol_cmd_xfer(inst, command, payload, payload_len,
					 response, &rx_len);
	if (ret)
		return ret;
	if (rx_len < response_len)
		return -EIO;

	return 0;
}

static void pdm_mcu_protocol_encode_index(u8 *buf, u32 index)
{
	pdm_mcu_protocol_encode_be32(buf, index);
}

int pdm_mcu_protocol_get_version(struct pdm_mcu_instance *inst,
				 struct pdm_mcu_version *version)
{
	u8 payload[sizeof(version->index)];

	pdm_mcu_protocol_encode_index(payload, version->index);
	return pdm_mcu_protocol_cmd_expect(inst,
		PDM_MCU_PROTOCOL_CMD_GET_VERSION, payload, sizeof(payload),
		(u8 *)version, sizeof(*version));
}

int pdm_mcu_protocol_get_status(struct pdm_mcu_instance *inst,
			       struct pdm_mcu_status *status)
{
	u8 payload[sizeof(status->index)];

	pdm_mcu_protocol_encode_index(payload, status->index);
	return pdm_mcu_protocol_cmd_expect(inst,
		PDM_MCU_PROTOCOL_CMD_GET_STATUS, payload, sizeof(payload),
		(u8 *)status, sizeof(*status));
}

int pdm_mcu_protocol_reset(struct pdm_mcu_instance *inst, u32 index)
{
	u8 payload[sizeof(index)];
	u32 rx_len = 0;

	pdm_mcu_protocol_encode_index(payload, index);
	return pdm_mcu_protocol_cmd_xfer(inst, PDM_MCU_PROTOCOL_CMD_RESET,
					 payload, sizeof(payload), NULL, &rx_len);
}

int pdm_mcu_protocol_command(struct pdm_mcu_instance *inst,
			     struct pdm_mcu_command *command)
{
	u32 rx_len = command->rx_len;
	int ret;

	if (command->tx_len > PDM_MCU_MAX_TRANSFER_SIZE ||
	    command->rx_len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	ret = pdm_mcu_protocol_cmd_xfer(inst, command->command,
					 command->tx_data, command->tx_len,
					 command->rx_data, &rx_len);
	if (ret)
		return ret;

	command->rx_len = rx_len;
	return 0;
}

int pdm_mcu_protocol_read_data(struct pdm_mcu_instance *inst,
			       struct pdm_mcu_data *data)
{
	struct pdm_mcu_xfer xfer = {
		.type = PDM_MCU_XFER_DATA_READ,
		.id = data->address,
		.rx = data->data,
		.rx_len = data->len,
	};
	int ret;

	ret = pdm_mcu_protocol_xfer(inst, &xfer);
	if (ret)
		return ret;
	if (xfer.actual_rx_len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	data->address = xfer.id;
	data->len = xfer.actual_rx_len;
	return 0;
}

int pdm_mcu_protocol_write_data(struct pdm_mcu_instance *inst,
				const struct pdm_mcu_data *data)
{
	struct pdm_mcu_xfer xfer = {
		.type = PDM_MCU_XFER_DATA_WRITE,
		.id = data->address,
		.tx = data->data,
		.tx_len = data->len,
	};

	return pdm_mcu_protocol_xfer(inst, &xfer);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PDM MCU protocol layer");
