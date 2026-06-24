// SPDX-License-Identifier: GPL-2.0
/**
 * @file pdm_mcu_uart_serdev.c
 * @brief UART serdev transport binding for PDM MCU devices
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/serdev.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/wait.h>

#include "pdm_mcu_internal.h"
#include "osal.h"

static const struct of_device_id pdm_mcu_serdev_of_match[] = {
	{ .compatible = "pdm,mcu-uart" },
	{ .compatible = "vendor,pdm-mcu-uart" },
	{ }
};
MODULE_DEVICE_TABLE(of, pdm_mcu_serdev_of_match);

static size_t pdm_mcu_serdev_receive_buf(struct serdev_device *serdev,
					 const u8 *data, size_t count);
static int pdm_mcu_serdev_probe(struct serdev_device *serdev);
static void pdm_mcu_serdev_remove(struct serdev_device *serdev);

static const struct serdev_device_ops pdm_mcu_serdev_ops = {
	.receive_buf = pdm_mcu_serdev_receive_buf,
};

static struct serdev_device_driver pdm_mcu_serdev_driver = {
	.probe = pdm_mcu_serdev_probe,
	.remove = pdm_mcu_serdev_remove,
	.driver = {
		.name = "pdm-mcu-serdev",
		.of_match_table = pdm_mcu_serdev_of_match,
	},
};

static unsigned long pdm_mcu_uart_serdev_deadline(u32 timeout_ms)
{
	return jiffies + msecs_to_jiffies(timeout_ms ? timeout_ms : 1U);
}

static size_t pdm_mcu_serdev_receive_buf(struct serdev_device *serdev,
					 const u8 *data, size_t count)
{
	struct pdm_mcu_bus_device *bus_dev = serdev_device_get_drvdata(serdev);
	struct pdm_mcu_instance *inst;
	size_t copied;

	if (!bus_dev || !bus_dev->inst)
		return 0;

	inst = bus_dev->inst;
	copied = kfifo_in_spinlocked(&inst->transport.uart.rx_fifo, data, count,
				       &inst->transport.uart.rx_lock);
	if (copied)
		wake_up_interruptible(&inst->transport.uart.rx_wait);
	return copied;
}

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

static int pdm_mcu_serdev_probe(struct serdev_device *serdev)
{
	struct pdm_mcu_bus_device *bus_dev;

	bus_dev = devm_kzalloc(&serdev->dev, sizeof(*bus_dev), GFP_KERNEL);
	if (!bus_dev)
		return -ENOMEM;

	bus_dev->bus.serdev = serdev;
	serdev_device_set_drvdata(serdev, bus_dev);
	return pdm_mcu_register_bus_device(&serdev->dev,
					   PDM_MCU_BACKEND_UART, bus_dev);
}

static void pdm_mcu_serdev_remove(struct serdev_device *serdev)
{
	struct pdm_mcu_bus_device *bus_dev = serdev_device_get_drvdata(serdev);

	pdm_mcu_unregister_bus_device(bus_dev);
	serdev_device_set_drvdata(serdev, NULL);
}

int pdm_mcu_uart_write_bus(struct pdm_mcu_instance *inst,
			   const u8 *buf, size_t len)
{
	struct serdev_device *serdev = inst->transport.uart.serdev;
	unsigned long deadline;
	size_t done = 0;

	if (!serdev)
		return -ENODEV;

	deadline = pdm_mcu_uart_serdev_deadline(
		inst->transport.uart.rx_timeout_ms);
	while (done < len) {
		int ret;

		ret = serdev_device_write_buf(serdev, buf + done, len - done);
		if (ret > 0) {
			done += ret;
			serdev_device_wait_until_sent(serdev,
				msecs_to_jiffies(
					inst->transport.uart.rx_timeout_ms));
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

int pdm_mcu_uart_read_bus(struct pdm_mcu_instance *inst, u8 *buf,
			  size_t len)
{
	unsigned long deadline;
	size_t done = 0;

	if (!inst->transport.uart.serdev)
		return -ENODEV;
	if (!len)
		return 0;

	deadline = pdm_mcu_uart_serdev_deadline(
		inst->transport.uart.rx_timeout_ms);
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

int pdm_mcu_uart_setup_bus(struct pdm_mcu_instance *inst)
{
	struct device_node *np = inst->pdm_dev->dev.of_node;
	struct pdm_mcu_bus_device *bus_dev = inst->pdm_dev->config_data;
	struct serdev_device *serdev;
	u32 value;
	int ret;

	if (!bus_dev || bus_dev->type != PDM_MCU_BACKEND_UART ||
	    !bus_dev->bus.serdev)
		return -ENODEV;

	serdev = bus_dev->bus.serdev;
	if (!serdev)
		return -ENODEV;

	inst->transport.uart.serdev = serdev;
	inst->transport.uart.bus_dev = bus_dev;
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

	bus_dev->inst = inst;
	serdev_device_set_client_ops(serdev, &pdm_mcu_serdev_ops);
	ret = serdev_device_open(serdev);
	if (ret) {
		bus_dev->inst = NULL;
		serdev_device_set_client_ops(serdev, NULL);
		return ret;
	}

	value = serdev_device_set_baudrate(serdev,
					 inst->transport.uart.baudrate);
	if (value)
		inst->transport.uart.baudrate = value;
	serdev_device_set_flow_control(serdev,
				       pdm_mcu_uart_hw_flow_control(np));

	LOG_INFO("Opened serdev %s baud=%u",
		 dev_name(&serdev->dev), inst->transport.uart.baudrate);
	return 0;
}

void pdm_mcu_uart_cleanup_bus(struct pdm_mcu_instance *inst)
{
	struct serdev_device *serdev = inst->transport.uart.serdev;
	struct pdm_mcu_bus_device *bus_dev = inst->transport.uart.bus_dev;

	if (!serdev)
		return;

	if (bus_dev)
		bus_dev->inst = NULL;
	wake_up_interruptible(&inst->transport.uart.rx_wait);
	serdev_device_close(serdev);
	serdev_device_set_client_ops(serdev, NULL);
	inst->transport.uart.serdev = NULL;
	inst->transport.uart.bus_dev = NULL;
}

int pdm_mcu_uart_driver_register(void)
{
	return serdev_device_driver_register(&pdm_mcu_serdev_driver);
}

void pdm_mcu_uart_driver_unregister(void)
{
	serdev_device_driver_unregister(&pdm_mcu_serdev_driver);
}
