// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_uart.c
 * @brief UART transports for PDM MCU devices
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/string.h>

#include "pdm/core/driver/pdm_backend.h"
#include "pdm_mcu_internal.h"
#include "osal.h"

static const struct of_device_id pdm_mcu_uart_of_match[] = {
	{ .compatible = "pdm,mcu-uart" },
	{ .compatible = "vendor,pdm-mcu-uart" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_uart_of_match);

static int pdm_mcu_uart_setup(struct pdm_mcu_instance *inst);
static void pdm_mcu_uart_cleanup(struct pdm_mcu_instance *inst);
static int pdm_mcu_uart_xfer(struct pdm_mcu_instance *inst,
			     struct pdm_mcu_xfer *xfer);
static int pdm_mcu_uart_cmd_xfer(struct pdm_mcu_instance *inst, u32 command,
				 const u8 *tx, u32 tx_len, u8 *rx, u32 *rx_len);
static int pdm_mcu_uart_data_read(struct pdm_mcu_instance *inst,
				  u32 *address, u8 *buf, u32 *len);
static int pdm_mcu_uart_data_write(struct pdm_mcu_instance *inst, u32 address,
				   const u8 *buf, u32 len);

static const struct pdm_mcu_transport_ops pdm_mcu_uart_ops = {
	.name = "uart",
	.capability = PDM_CTL_DEVICE_CAP_TRANSPORT_UART,
	.max_tx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.max_rx_size = PDM_MCU_MAX_TRANSFER_SIZE,
	.setup = pdm_mcu_uart_setup,
	.cleanup = pdm_mcu_uart_cleanup,
	.xfer = pdm_mcu_uart_xfer,
};

static void pdm_mcu_uart_encode_be32(u8 *buf, u32 value)
{
	buf[0] = (u8)(value >> 24);
	buf[1] = (u8)(value >> 16);
	buf[2] = (u8)(value >> 8);
	buf[3] = (u8)value;
}

static int pdm_mcu_uart_write_bytes(struct pdm_mcu_instance *inst,
				    const u8 *buf, size_t len)
{
	return pdm_mcu_uart_write_bus(inst, buf, len);
}

static int pdm_mcu_uart_read_bytes(struct pdm_mcu_instance *inst,
				   u8 *buf, size_t len)
{
	return pdm_mcu_uart_read_bus(inst, buf, len);
}

static int pdm_mcu_uart_setup(struct pdm_mcu_instance *inst)
{
	return pdm_mcu_uart_setup_bus(inst);
}

static void pdm_mcu_uart_cleanup(struct pdm_mcu_instance *inst)
{
	pdm_mcu_uart_cleanup_bus(inst);
}

static int pdm_mcu_uart_write_exact(struct pdm_mcu_instance *inst,
				    const u8 *buf, u32 len)
{
	int ret;

	ret = pdm_mcu_uart_write_bytes(inst, buf, len);
	if (ret < 0)
		return ret;

	return ret == len ? 0 : -EIO;
}

static int pdm_mcu_uart_cmd_xfer(struct pdm_mcu_instance *inst, u32 command,
				 const u8 *tx, u32 tx_len, u8 *rx, u32 *rx_len)
{
	u8 buf[PDM_MCU_MAX_TRANSFER_SIZE + PDM_MCU_TRANSPORT_ID_BYTES];
	u32 expect = rx_len ? *rx_len : 0;
	int ret;

	if (tx_len + PDM_MCU_TRANSPORT_ID_BYTES > sizeof(buf))
		return -EMSGSIZE;
	if (expect > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	pdm_mcu_uart_encode_be32(buf, command);
	if (tx_len)
		memcpy(buf + PDM_MCU_TRANSPORT_ID_BYTES, tx, tx_len);

	ret = pdm_mcu_uart_write_exact(inst, buf,
					tx_len + PDM_MCU_TRANSPORT_ID_BYTES);
	if (ret)
		return ret;
	if (!expect) {
		if (rx_len)
			*rx_len = 0;
		return 0;
	}

	ret = pdm_mcu_uart_read_bytes(inst, rx, expect);
	if (ret < 0)
		return ret;
	if (rx_len)
		*rx_len = ret;
	return 0;
}

static int pdm_mcu_uart_data_read(struct pdm_mcu_instance *inst,
				  u32 *address, u8 *buf, u32 *len)
{
	int ret;

	(void)address;
	ret = pdm_mcu_uart_read_bytes(inst, buf, *len);
	if (ret < 0)
		return ret;

	*len = ret;
	return 0;
}

static int pdm_mcu_uart_data_write(struct pdm_mcu_instance *inst, u32 address,
				   const u8 *buf, u32 len)
{
	(void)address;
	return pdm_mcu_uart_write_exact(inst, buf, len);
}

static int pdm_mcu_uart_xfer(struct pdm_mcu_instance *inst,
			     struct pdm_mcu_xfer *xfer)
{
	u32 len;
	int ret;

	switch (xfer->type) {
	case PDM_MCU_XFER_CMD:
		len = xfer->rx_len;
		ret = pdm_mcu_uart_cmd_xfer(inst, xfer->id, xfer->tx,
					    xfer->tx_len, xfer->rx, &len);
		if (ret)
			return ret;
		xfer->actual_rx_len = len;
		return 0;
	case PDM_MCU_XFER_DATA_READ:
		len = xfer->rx_len;
		ret = pdm_mcu_uart_data_read(inst, &xfer->id, xfer->rx, &len);
		if (ret)
			return ret;
		xfer->actual_rx_len = len;
		return 0;
	case PDM_MCU_XFER_DATA_WRITE:
		return pdm_mcu_uart_data_write(inst, xfer->id, xfer->tx,
					       xfer->tx_len);
	default:
		return -EINVAL;
	}
}

int __weak pdm_mcu_uart_write_bus(struct pdm_mcu_instance *inst,
				   const u8 *buf, size_t len)
{
	(void)inst;
	(void)buf;
	(void)len;
	return -ENODEV;
}

int __weak pdm_mcu_uart_read_bus(struct pdm_mcu_instance *inst, u8 *buf,
				  size_t len)
{
	(void)inst;
	(void)buf;
	(void)len;
	return -ENODEV;
}

int __weak pdm_mcu_uart_setup_bus(struct pdm_mcu_instance *inst)
{
	(void)inst;
	return -ENODEV;
}

void __weak pdm_mcu_uart_cleanup_bus(struct pdm_mcu_instance *inst)
{
	(void)inst;
}

int __weak pdm_mcu_uart_driver_register(void)
{
	return 0;
}

void __weak pdm_mcu_uart_driver_unregister(void)
{
}

pdm_backend_register(mcu_uart, PDM_CTL_DEVICE_TYPE_MCU,
		     PDM_BACKEND_CLASS_TRANSPORT, pdm_mcu_uart_of_match,
		     &pdm_mcu_uart_ops, pdm_mcu_uart_driver_register,
		     pdm_mcu_uart_driver_unregister);
