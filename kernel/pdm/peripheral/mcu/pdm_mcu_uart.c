// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_uart.c
 * @brief UART transports for PDM MCU devices
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
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

static unsigned long pdm_mcu_uart_deadline(u32 timeout_ms)
{
	return jiffies + msecs_to_jiffies(timeout_ms ? timeout_ms : 1U);
}

static int pdm_mcu_uart_write_file(struct pdm_mcu_instance *inst,
				   const u8 *buf, size_t len)
{
	struct file *file = inst->transport.uart.file;
	unsigned long deadline;
	loff_t pos = 0;
	size_t done = 0;

	if (!file)
		return -ENODEV;

	deadline = pdm_mcu_uart_deadline(inst->transport.uart.rx_timeout_ms);
	while (done < len) {
		ssize_t ret;

		ret = kernel_write(file, buf + done, len - done, &pos);
		if (ret > 0) {
			done += ret;
			continue;
		}
		if (ret && ret != -EAGAIN)
			return ret;
		if (time_after(jiffies, deadline))
			return done ? (int)done : -ETIMEDOUT;
		msleep(1);
	}

	return (int)done;
}

static int pdm_mcu_uart_read_file(struct pdm_mcu_instance *inst,
				  u8 *buf, size_t len)
{
	struct file *file = inst->transport.uart.file;
	unsigned long deadline;
	loff_t pos = 0;
	size_t done = 0;

	if (!file)
		return -ENODEV;
	if (!len)
		return 0;

	deadline = pdm_mcu_uart_deadline(inst->transport.uart.rx_timeout_ms);
	while (done < len) {
		ssize_t ret;

		ret = kernel_read(file, buf + done, len - done, &pos);
		if (ret > 0) {
			done += ret;
			continue;
		}
		if (ret && ret != -EAGAIN)
			return ret;
		if (time_after(jiffies, deadline))
			return done ? (int)done : -ETIMEDOUT;
		msleep(1);
	}

	return (int)done;
}

static int pdm_mcu_uart_write_bytes(struct pdm_mcu_instance *inst,
				    const u8 *buf, size_t len)
{
	if (inst->transport.uart.use_serdev)
		return pdm_mcu_uart_write_native(inst, buf, len);

	return pdm_mcu_uart_write_file(inst, buf, len);
}

static int pdm_mcu_uart_read_bytes(struct pdm_mcu_instance *inst,
				   u8 *buf, size_t len)
{
	if (inst->transport.uart.use_serdev)
		return pdm_mcu_uart_read_native(inst, buf, len);

	return pdm_mcu_uart_read_file(inst, buf, len);
}

static int pdm_mcu_uart_setup_file(struct pdm_mcu_instance *inst)
{
	struct device_node *np = inst->pdm_dev->dev.of_node;
	const char *path;
	u32 value;
	int ret;

	if (!np)
		return -EINVAL;

	ret = of_property_read_string(np, "tty-path", &path);
	if (ret)
		ret = of_property_read_string(np, "uart-path", &path);
	if (ret)
		ret = of_property_read_string(np, "device-path", &path);
	if (ret)
		return ret;

	strscpy(inst->transport.uart.path, path,
		sizeof(inst->transport.uart.path));
	inst->transport.uart.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	if (!of_property_read_u32(np, "rx-timeout-ms", &value))
		inst->transport.uart.rx_timeout_ms = value;
	if (!of_property_read_u32(np, "baudrate", &value))
		inst->transport.uart.baudrate = value;

	inst->transport.uart.file = filp_open(inst->transport.uart.path,
					       O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
	if (IS_ERR(inst->transport.uart.file)) {
		ret = PTR_ERR(inst->transport.uart.file);
		inst->transport.uart.file = NULL;
		LOG_ERROR("Failed to open %s: %d",
			  inst->transport.uart.path, ret);
		return ret;
	}

	LOG_INFO("Opened %s%s%u",
		 inst->transport.uart.path,
		 inst->transport.uart.baudrate ? " baud=" : "",
		 inst->transport.uart.baudrate);
	return 0;
}

static int pdm_mcu_uart_setup(struct pdm_mcu_instance *inst)
{
	int ret;

	ret = pdm_mcu_uart_setup_native(inst);
	if (!ret)
		return 0;
	if (ret != -ENODEV)
		return ret;

	return pdm_mcu_uart_setup_file(inst);
}

static void pdm_mcu_uart_cleanup(struct pdm_mcu_instance *inst)
{
	if (inst->transport.uart.use_serdev) {
		pdm_mcu_uart_cleanup_native(inst);
		return;
	}

	if (!inst->transport.uart.file)
		return;

	filp_close(inst->transport.uart.file, NULL);
	inst->transport.uart.file = NULL;
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

int __weak pdm_mcu_uart_write_native(struct pdm_mcu_instance *inst,
				      const u8 *buf, size_t len)
{
	(void)inst;
	(void)buf;
	(void)len;
	return -ENODEV;
}

int __weak pdm_mcu_uart_read_native(struct pdm_mcu_instance *inst, u8 *buf,
				     size_t len)
{
	(void)inst;
	(void)buf;
	(void)len;
	return -ENODEV;
}

int __weak pdm_mcu_uart_setup_native(struct pdm_mcu_instance *inst)
{
	(void)inst;
	return -ENODEV;
}

void __weak pdm_mcu_uart_cleanup_native(struct pdm_mcu_instance *inst)
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
