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

#include "pdm/core/pdm_client.h"
#include "pdm/core/pdm_device.h"
#include "pdm/pdm_mcu.h"

#define PDM_MCU_DEFAULT_RX_TIMEOUT_MS 100U
#define PDM_MCU_DEFAULT_BAUDRATE 115200U
#define PDM_MCU_UART_PATH_LEN 96U
#define PDM_MCU_UART_RX_FIFO_SIZE 512U
#define PDM_MCU_CAN_IFNAME_LEN 16U
#define PDM_MCU_MAX_PREFIX_BYTES 4U

enum pdm_mcu_backend_type {
	PDM_MCU_BACKEND_MEMORY = 0,
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
int pdm_mcu_register_native_device(struct device *parent,
				   enum pdm_mcu_backend_type type,
				   struct pdm_mcu_native_device *native);
void pdm_mcu_unregister_native_device(struct pdm_mcu_native_device *native);

extern const struct pdm_mcu_transport_ops pdm_mcu_uart_ops;
extern const struct pdm_mcu_transport_ops pdm_mcu_can_ops;

#if IS_ENABLED(CONFIG_PDM_MCU_I2C) && IS_ENABLED(CONFIG_I2C)
extern const struct pdm_mcu_transport_ops pdm_mcu_i2c_ops;
int pdm_mcu_i2c_driver_register(void);
void pdm_mcu_i2c_driver_unregister(void);
#else
static inline int pdm_mcu_i2c_driver_register(void)
{
	return 0;
}
static inline void pdm_mcu_i2c_driver_unregister(void)
{
}
#endif

#if IS_ENABLED(CONFIG_PDM_MCU_SPI) && IS_ENABLED(CONFIG_SPI)
extern const struct pdm_mcu_transport_ops pdm_mcu_spi_ops;
int pdm_mcu_spi_driver_register(void);
void pdm_mcu_spi_driver_unregister(void);
#else
static inline int pdm_mcu_spi_driver_register(void)
{
	return 0;
}
static inline void pdm_mcu_spi_driver_unregister(void)
{
}
#endif

#if IS_ENABLED(CONFIG_PDM_MCU_UART_SERDEV) && IS_ENABLED(CONFIG_SERIAL_DEV_BUS)
int pdm_mcu_serdev_driver_register(void);
void pdm_mcu_serdev_driver_unregister(void);
#else
static inline int pdm_mcu_serdev_driver_register(void)
{
	return 0;
}
static inline void pdm_mcu_serdev_driver_unregister(void)
{
}
#endif

#endif /* PDM_MCU_INTERNAL_H */
