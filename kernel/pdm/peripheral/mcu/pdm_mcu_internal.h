// SPDX-License-Identifier: GPL-2.0

#ifndef PDM_MCU_INTERNAL_H
#define PDM_MCU_INTERNAL_H

#include <linux/fs.h>
#include <linux/kfifo.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "pdm/core/chardev/pdm_client.h"
#include "pdm/core/device/pdm_device.h"
#include "pdm/pdm_mcu.h"

#define PDM_MCU_DEFAULT_RX_TIMEOUT_MS 100U
#define PDM_MCU_DEFAULT_BAUDRATE 115200U
#define PDM_MCU_UART_PATH_LEN 96U
#define PDM_MCU_UART_RX_FIFO_SIZE 512U
#define PDM_MCU_CAN_IFNAME_LEN 16U
#define PDM_MCU_MAX_PREFIX_BYTES 4U

enum pdm_mcu_backend_type {
	PDM_MCU_BACKEND_UART,
	PDM_MCU_BACKEND_CAN,
	PDM_MCU_BACKEND_I2C,
	PDM_MCU_BACKEND_SPI,
};

struct device;
struct i2c_client;
struct serdev_device;
struct socket;
struct spi_device;
struct pdm_mcu_instance;

struct pdm_mcu_transport_ops {
	enum pdm_mcu_backend_type type;
	const char *name;
	u64 capability;
	int (*setup)(struct pdm_mcu_instance *inst);
	void (*cleanup)(struct pdm_mcu_instance *inst);
	int (*reset)(struct pdm_mcu_instance *inst);
	int (*command)(struct pdm_mcu_instance *inst,
			 struct pdm_mcu_command *command);
	int (*read_data)(struct pdm_mcu_instance *inst,
			 struct pdm_mcu_data *data);
	int (*write_data)(struct pdm_mcu_instance *inst,
			  const struct pdm_mcu_data *data);
};

struct pdm_mcu_native_device {
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
	struct pdm_client client;
	struct pdm_device *pdm_dev;
	struct mutex lock;
	const struct pdm_mcu_transport_ops *ops;
	ktime_t start_time;
	bool online;
	u32 state;
	s32 last_error;
	union {
		struct {
			struct file *file;
			struct serdev_device *serdev;
			struct pdm_mcu_native_device *native;
			DECLARE_KFIFO(rx_fifo, u8, PDM_MCU_UART_RX_FIFO_SIZE);
			spinlock_t rx_lock;
			wait_queue_head_t rx_wait;
			char path[PDM_MCU_UART_PATH_LEN];
			u32 baudrate;
			u32 rx_timeout_ms;
			bool use_serdev;
		} uart;
		struct {
			struct socket *sock;
			char ifname[PDM_MCU_CAN_IFNAME_LEN];
			u32 rx_timeout_ms;
			bool extended_id;
		} can;
		struct {
			struct i2c_client *client;
			u32 rx_timeout_ms;
			u8 command_bytes;
			u8 address_bytes;
		} i2c;
		struct {
			struct spi_device *spi;
			u32 rx_timeout_ms;
			u8 command_bytes;
			u8 address_bytes;
		} spi;
	} transport;
};

const struct pdm_mcu_transport_ops *pdm_mcu_transport_select(const char *compatible);

int pdm_mcu_uart_setup_native(struct pdm_mcu_instance *inst);
void pdm_mcu_uart_cleanup_native(struct pdm_mcu_instance *inst);
int pdm_mcu_uart_write_native(struct pdm_mcu_instance *inst,
			      const u8 *buf, size_t len);
int pdm_mcu_uart_read_native(struct pdm_mcu_instance *inst, u8 *buf,
			     size_t len);
int pdm_mcu_uart_driver_register(void);
void pdm_mcu_uart_driver_unregister(void);
int pdm_mcu_register_native_device(struct device *parent,
				   enum pdm_mcu_backend_type type,
				   struct pdm_mcu_native_device *native);
void pdm_mcu_unregister_native_device(struct pdm_mcu_native_device *native);

#endif /* PDM_MCU_INTERNAL_H */
