// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_MCU_INTERNAL_H
#define PDM_MCU_INTERNAL_H

#include <linux/kfifo.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "pdm/core/chardev/pdm_client.h"
#include "pdm/core/device/pdm_device.h"
#include "pdm/core/driver/pdm_driver.h"
#include "pdm/pdm_mcu.h"

#define PDM_MCU_DEFAULT_RX_TIMEOUT_MS 100U
#define PDM_MCU_DEFAULT_BAUDRATE 115200U
#define PDM_MCU_UART_RX_FIFO_SIZE 512U
#define PDM_MCU_MAX_PREFIX_BYTES 4U
#define PDM_MCU_TRANSPORT_ID_BYTES 4U

enum pdm_mcu_backend_type {
	PDM_MCU_BACKEND_UART,
	PDM_MCU_BACKEND_CAN,
	PDM_MCU_BACKEND_I2C,
	PDM_MCU_BACKEND_SPI,
};

struct device;
struct i2c_client;
struct net_device;
struct serdev_device;
struct socket;
struct spi_device;
struct pdm_mcu_instance;

struct pdm_mcu_xfer {
	u32 command;
	const u8 *tx;
	u32 tx_len;
	u8 *rx;
	u32 rx_len;
	u32 actual_rx_len;
};

struct pdm_mcu_transport_ops {
	const char *name;
	u64 capability;
	u32 max_tx_size;
	u32 max_rx_size;
	int (*setup)(struct pdm_mcu_instance *inst);
	void (*cleanup)(struct pdm_mcu_instance *inst);
	int (*xfer)(struct pdm_mcu_instance *inst, struct pdm_mcu_xfer *xfer);
};

struct pdm_mcu_bus_device {
	enum pdm_mcu_backend_type type;
	struct pdm_device *pdm_dev;
	struct pdm_mcu_instance *inst;
	union {
		struct serdev_device *serdev;
		struct i2c_client *i2c;
		struct spi_device *spi;
	} bus;
};

struct pdm_mcu_instance {
	struct pdm_driver_instance base;
	const struct pdm_mcu_transport_ops *ops;
	ktime_t start_time;
	u32 state;
	s32 last_error;
	union {
		struct {
			struct serdev_device *serdev;
			struct pdm_mcu_bus_device *bus_dev;
			DECLARE_KFIFO(rx_fifo, u8, PDM_MCU_UART_RX_FIFO_SIZE);
			spinlock_t rx_lock;
			wait_queue_head_t rx_wait;
			u32 baudrate;
			u32 rx_timeout_ms;
		} uart;
		struct {
			struct socket *sock;
			struct net_device *netdev;
			u32 rx_timeout_ms;
			bool extended_id;
		} can;
		struct {
			struct i2c_client *client;
			u32 rx_timeout_ms;
			u8 command_bytes;
		} i2c;
		struct {
			struct spi_device *spi;
			u32 rx_timeout_ms;
			u8 command_bytes;
		} spi;
	} transport;
};

int pdm_mcu_protocol_get_version(struct pdm_mcu_instance *inst,
				 struct pdm_mcu_version *version);
int pdm_mcu_protocol_get_status(struct pdm_mcu_instance *inst,
			       struct pdm_mcu_status *status);
int pdm_mcu_protocol_reset(struct pdm_mcu_instance *inst, u32 index);
int pdm_mcu_protocol_command(struct pdm_mcu_instance *inst,
			     struct pdm_mcu_command *command);

int pdm_mcu_uart_setup_bus(struct pdm_mcu_instance *inst);
void pdm_mcu_uart_cleanup_bus(struct pdm_mcu_instance *inst);
int pdm_mcu_uart_write_bus(struct pdm_mcu_instance *inst,
			   const u8 *buf, size_t len);
int pdm_mcu_uart_read_bus(struct pdm_mcu_instance *inst, u8 *buf,
			  size_t len);
int pdm_mcu_uart_driver_register(void);
void pdm_mcu_uart_driver_unregister(void);
int pdm_mcu_register_bus_device(struct device *parent,
				enum pdm_mcu_backend_type type,
				struct pdm_mcu_bus_device *bus_dev);
void pdm_mcu_unregister_bus_device(struct pdm_mcu_bus_device *bus_dev);

#endif /* PDM_MCU_INTERNAL_H */
