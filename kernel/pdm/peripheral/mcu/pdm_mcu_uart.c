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
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/wait.h>

#if IS_ENABLED(CONFIG_PDM_MCU_UART_SERDEV) && IS_ENABLED(CONFIG_SERIAL_DEV_BUS)
#include <linux/serdev.h>
#endif

#include "pdm/core/pdm_backend.h"
#include "pdm_mcu_internal.h"
#include "osal.h"

static unsigned long pdm_mcu_uart_deadline(u32 timeout_ms)
{
	return jiffies + msecs_to_jiffies(timeout_ms ? timeout_ms : 1U);
}

static const struct of_device_id pdm_mcu_uart_of_match[] = {
	{ .compatible = "pdm,mcu-uart" },
	{ .compatible = "vendor,pdm-mcu-uart" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_uart_of_match);

#if IS_ENABLED(CONFIG_PDM_MCU_UART_SERDEV) && IS_ENABLED(CONFIG_SERIAL_DEV_BUS)
static size_t pdm_mcu_serdev_receive_buf(struct serdev_device *serdev,
						 const u8 *data, size_t count)
{
	struct pdm_mcu_native_device *native = serdev_device_get_drvdata(serdev);
	struct pdm_mcu_instance *inst;
	size_t copied;

	if (!native || !native->inst)
		return 0;

	inst = native->inst;
	copied = kfifo_in_spinlocked(&inst->transport.uart.rx_fifo, data, count,
				       &inst->transport.uart.rx_lock);
	if (copied)
		wake_up_interruptible(&inst->transport.uart.rx_wait);
	return copied;
}

static const struct serdev_device_ops pdm_mcu_serdev_ops = {
	.receive_buf = pdm_mcu_serdev_receive_buf,
};

static bool pdm_mcu_uart_hw_flow_control(struct device_node *np)
{
	const char *flow;

	if (!np)
		return false;
	if (of_property_read_bool(np, "uart-has-rtscts"))
		return true;
	if (!of_property_read_string(np, "flow-control", &flow) &&
	    !strcmp(flow, "hw"))
		return true;
	return false;
}

static int pdm_mcu_uart_write_serdev(struct pdm_mcu_instance *inst,
				     const u8 *buf, size_t len)
{
	struct serdev_device *serdev = inst->transport.uart.serdev;
	unsigned long deadline;
	size_t done = 0;

	if (!serdev)
		return -ENODEV;

	deadline = pdm_mcu_uart_deadline(inst->transport.uart.rx_timeout_ms);
	while (done < len) {
		int ret;

		ret = serdev_device_write_buf(serdev, buf + done, len - done);
		if (ret > 0) {
			done += ret;
			serdev_device_wait_until_sent(serdev,
						       msecs_to_jiffies(inst->transport.uart.rx_timeout_ms));
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

static int pdm_mcu_uart_read_serdev(struct pdm_mcu_instance *inst,
				    u8 *buf, size_t len)
{
	unsigned long deadline;
	size_t done = 0;

	if (!inst->transport.uart.serdev)
		return -ENODEV;
	if (!len)
		return 0;

	deadline = pdm_mcu_uart_deadline(inst->transport.uart.rx_timeout_ms);
	while (done < len) {
		unsigned int copied;
		long wait_left;
		long ret;

		copied = kfifo_out_spinlocked(&inst->transport.uart.rx_fifo,
					      buf + done, len - done,
					      &inst->transport.uart.rx_lock);
		if (copied) {
			done += copied;
			continue;
		}

		if (time_after_eq(jiffies, deadline))
			return done ? (int)done : -ETIMEDOUT;

		wait_left = deadline - jiffies;
		if (wait_left <= 0)
			wait_left = 1;
		ret = wait_event_interruptible_timeout(inst->transport.uart.rx_wait,
			!kfifo_is_empty(&inst->transport.uart.rx_fifo) ||
			!inst->online,
			wait_left);
		if (ret < 0)
			return ret;
		if (!ret && time_after_eq(jiffies, deadline))
			return done ? (int)done : -ETIMEDOUT;
	}

	return (int)done;
}

static int pdm_mcu_uart_setup_serdev(struct pdm_mcu_instance *inst,
				     struct pdm_mcu_native_device *native)
{
	struct device_node *np = inst->pdm_dev->dev.of_node;
	struct serdev_device *serdev = native->bus.serdev;
	u32 value;
	int ret;

	if (!serdev)
		return -ENODEV;

	inst->transport.uart.serdev = serdev;
	inst->transport.uart.native = native;
	inst->transport.uart.use_serdev = true;
	inst->transport.uart.rx_timeout_ms = PDM_MCU_DEFAULT_RX_TIMEOUT_MS;
	inst->transport.uart.baudrate = PDM_MCU_DEFAULT_BAUDRATE;
	INIT_KFIFO(inst->transport.uart.rx_fifo);
	spin_lock_init(&inst->transport.uart.rx_lock);
	init_waitqueue_head(&inst->transport.uart.rx_wait);

	if (np) {
		if (!of_property_read_u32(np, "rx-timeout-ms", &value))
			inst->transport.uart.rx_timeout_ms = value;
		if (!of_property_read_u32(np, "current-speed", &value) ||
		    !of_property_read_u32(np, "baudrate", &value))
			inst->transport.uart.baudrate = value;
	}

	native->inst = inst;
	serdev_device_set_client_ops(serdev, &pdm_mcu_serdev_ops);
	ret = serdev_device_open(serdev);
	if (ret) {
		native->inst = NULL;
		serdev_device_set_client_ops(serdev, NULL);
		return ret;
	}

	value = serdev_device_set_baudrate(serdev,
					 inst->transport.uart.baudrate);
	if (value)
		inst->transport.uart.baudrate = value;
	serdev_device_set_flow_control(serdev,
				       pdm_mcu_uart_hw_flow_control(np));

	LOG_INFO("PDM-MCU-UART", "Opened serdev %s baud=%u",
		 dev_name(&serdev->dev), inst->transport.uart.baudrate);
	return 0;
}

static void pdm_mcu_uart_cleanup_serdev(struct pdm_mcu_instance *inst)
{
	struct serdev_device *serdev = inst->transport.uart.serdev;
	struct pdm_mcu_native_device *native = inst->transport.uart.native;

	if (!serdev)
		return;

	if (native)
		native->inst = NULL;
	wake_up_interruptible(&inst->transport.uart.rx_wait);
	serdev_device_close(serdev);
	serdev_device_set_client_ops(serdev, NULL);
	inst->transport.uart.serdev = NULL;
	inst->transport.uart.native = NULL;
	inst->transport.uart.use_serdev = false;
}
#else
static int pdm_mcu_uart_write_serdev(struct pdm_mcu_instance *inst,
				     const u8 *buf, size_t len)
{
	(void)inst;
	(void)buf;
	(void)len;
	return -ENODEV;
}

static int pdm_mcu_uart_read_serdev(struct pdm_mcu_instance *inst,
				    u8 *buf, size_t len)
{
	(void)inst;
	(void)buf;
	(void)len;
	return -ENODEV;
}
#endif

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
		return pdm_mcu_uart_write_serdev(inst, buf, len);

	return pdm_mcu_uart_write_file(inst, buf, len);
}

static int pdm_mcu_uart_read_bytes(struct pdm_mcu_instance *inst,
				   u8 *buf, size_t len)
{
	if (inst->transport.uart.use_serdev)
		return pdm_mcu_uart_read_serdev(inst, buf, len);

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
		LOG_ERROR("PDM-MCU-UART", "Failed to open %s: %d",
			  inst->transport.uart.path, ret);
		return ret;
	}

	LOG_INFO("PDM-MCU-UART", "Opened %s%s%u",
		 inst->transport.uart.path,
		 inst->transport.uart.baudrate ? " baud=" : "",
		 inst->transport.uart.baudrate);
	return 0;
}

static int pdm_mcu_uart_setup(struct pdm_mcu_instance *inst)
{
#if IS_ENABLED(CONFIG_PDM_MCU_UART_SERDEV) && IS_ENABLED(CONFIG_SERIAL_DEV_BUS)
	struct pdm_mcu_native_device *native = inst->pdm_dev->config_data;

	if (native && native->type == PDM_MCU_BACKEND_UART && native->bus.serdev)
		return pdm_mcu_uart_setup_serdev(inst, native);
#endif

	return pdm_mcu_uart_setup_file(inst);
}

static void pdm_mcu_uart_cleanup(struct pdm_mcu_instance *inst)
{
	if (inst->transport.uart.use_serdev) {
#if IS_ENABLED(CONFIG_PDM_MCU_UART_SERDEV) && IS_ENABLED(CONFIG_SERIAL_DEV_BUS)
		pdm_mcu_uart_cleanup_serdev(inst);
#endif
		return;
	}

	if (!inst->transport.uart.file)
		return;

	filp_close(inst->transport.uart.file, NULL);
	inst->transport.uart.file = NULL;
}

static int pdm_mcu_uart_reset(struct pdm_mcu_instance *inst)
{
	(void)inst;
	return 0;
}

static int pdm_mcu_uart_command(struct pdm_mcu_instance *inst,
				struct pdm_mcu_command *command)
{
	int ret;

	if (command->tx_len > PDM_MCU_MAX_TRANSFER_SIZE ||
	    command->rx_len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	if (command->tx_len) {
		ret = pdm_mcu_uart_write_bytes(inst, command->tx_data,
					       command->tx_len);
		if (ret < 0)
			return ret;
	}

	if (!command->rx_len)
		return 0;

	ret = pdm_mcu_uart_read_bytes(inst, command->rx_data, command->rx_len);
	if (ret < 0)
		return ret;
	command->rx_len = ret;
	return 0;
}

static int pdm_mcu_uart_read_data(struct pdm_mcu_instance *inst,
				  struct pdm_mcu_data *data)
{
	int ret;

	if (data->len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	ret = pdm_mcu_uart_read_bytes(inst, data->data, data->len);
	if (ret < 0)
		return ret;
	data->len = ret;
	return 0;
}

static int pdm_mcu_uart_write_data(struct pdm_mcu_instance *inst,
				   const struct pdm_mcu_data *data)
{
	int ret;

	if (data->len > PDM_MCU_MAX_TRANSFER_SIZE)
		return -EMSGSIZE;

	ret = pdm_mcu_uart_write_bytes(inst, data->data, data->len);
	if (ret < 0)
		return ret;
	return ret == data->len ? 0 : -EIO;
}

const struct pdm_mcu_transport_ops pdm_mcu_uart_ops = {
	.type = PDM_MCU_BACKEND_UART,
	.name = "uart",
	.capability = PDM_CTL_DEVICE_CAP_TRANSPORT_UART,
	.setup = pdm_mcu_uart_setup,
	.cleanup = pdm_mcu_uart_cleanup,
	.reset = pdm_mcu_uart_reset,
	.command = pdm_mcu_uart_command,
	.read_data = pdm_mcu_uart_read_data,
	.write_data = pdm_mcu_uart_write_data,
};

pdm_backend_register(mcu_uart, PDM_CTL_DEVICE_TYPE_MCU,
		     PDM_BACKEND_CLASS_TRANSPORT, pdm_mcu_uart_of_match,
		     &pdm_mcu_uart_ops, pdm_mcu_serdev_driver_register,
		     pdm_mcu_serdev_driver_unregister);

#if IS_ENABLED(CONFIG_PDM_MCU_UART_SERDEV) && IS_ENABLED(CONFIG_SERIAL_DEV_BUS)
static int pdm_mcu_serdev_probe(struct serdev_device *serdev)
{
	struct pdm_mcu_native_device *native;

	native = devm_kzalloc(&serdev->dev, sizeof(*native), GFP_KERNEL);
	if (!native)
		return -ENOMEM;

	native->bus.serdev = serdev;
	serdev_device_set_drvdata(serdev, native);
	return pdm_mcu_register_native_device(&serdev->dev,
					      PDM_MCU_BACKEND_UART, native);
}

static void pdm_mcu_serdev_remove(struct serdev_device *serdev)
{
	struct pdm_mcu_native_device *native = serdev_device_get_drvdata(serdev);

	pdm_mcu_unregister_native_device(native);
	serdev_device_set_drvdata(serdev, NULL);
}

static struct serdev_device_driver pdm_mcu_serdev_driver = {
	.probe = pdm_mcu_serdev_probe,
	.remove = pdm_mcu_serdev_remove,
	.driver = {
		.name = "pdm-mcu-serdev",
		.of_match_table = pdm_mcu_uart_of_match,
	},
};

int pdm_mcu_serdev_driver_register(void)
{
	return serdev_device_driver_register(&pdm_mcu_serdev_driver);
}

void pdm_mcu_serdev_driver_unregister(void)
{
	serdev_device_driver_unregister(&pdm_mcu_serdev_driver);
}
#endif
