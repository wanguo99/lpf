// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_TYPES_H
#define LPF_HW_TYPES_H

#include "osal.h"

typedef enum {
	LPF_GPIO_DIR_INPUT = 0,
	LPF_GPIO_DIR_OUTPUT,
} lpf_gpio_direction_t;

typedef enum {
	LPF_GPIO_LEVEL_LOW = 0,
	LPF_GPIO_LEVEL_HIGH,
} lpf_gpio_level_t;

typedef enum {
	LPF_GPIO_EDGE_NONE = 0,
	LPF_GPIO_EDGE_RISING,
	LPF_GPIO_EDGE_FALLING,
	LPF_GPIO_EDGE_BOTH,
} lpf_gpio_edge_t;

typedef void (*lpf_gpio_irq_callback_t)(uint32_t gpio_num,
					lpf_gpio_level_t level,
					void *user_data);

typedef struct {
	lpf_gpio_direction_t direction;
	lpf_gpio_level_t initial_level;
	lpf_gpio_edge_t edge;
	lpf_gpio_irq_callback_t callback;
	void *user_data;
} lpf_gpio_config_t;

typedef void *lpf_hw_pwm_handle_t;
typedef void *lpf_pwm_handle_t;

typedef struct {
	const char *consumer;
	uint32_t period_ns;
	uint32_t duty_ns;
	bool enabled;
	bool polarity_inversed;
} lpf_pwm_config_t;

typedef struct {
	uint32_t period_ns;
	uint32_t duty_ns;
	bool enabled;
	bool polarity_inversed;
} lpf_pwm_state_t;

typedef void *lpf_hw_bus_i2c_handle_t;
typedef void *lpf_i2c_handle_t;

#define LPF_I2C_M_RD      0x0001
#define LPF_I2C_M_TEN     0x0010
#define LPF_I2C_M_NOSTART 0x4000

typedef struct {
	const char *device;
	uint32_t timeout;
} lpf_i2c_config_t;

typedef struct {
	uint16_t addr;
	uint16_t flags;
	uint16_t len;
	uint8_t *buf;
} lpf_i2c_msg_t;

typedef void *lpf_hw_bus_spi_handle_t;
typedef void *lpf_spi_handle_t;

#define LPF_SPI_MODE_0 0x00
#define LPF_SPI_MODE_1 0x01
#define LPF_SPI_MODE_2 0x02
#define LPF_SPI_MODE_3 0x03

typedef struct {
	const char *device;
	uint8_t mode;
	uint8_t bits_per_word;
	uint32_t max_speed_hz;
	uint32_t timeout;
} lpf_spi_config_t;

typedef struct {
	const uint8_t *tx_buf;
	uint8_t *rx_buf;
	uint32_t len;
	uint32_t speed_hz;
	uint16_t delay_usecs;
	uint8_t bits_per_word;
	uint8_t cs_change;
} lpf_spi_transfer_t;

typedef void *lpf_hw_transport_can_handle_t;
typedef void *lpf_can_handle_t;

#define LPF_CAN_DEFAULT_INTERFACE "can0"
#define LPF_CAN_DEFAULT_BAUDRATE 500000
#define LPF_CAN_MAX_DATA_LEN 8

typedef struct {
	const char *interface;
	uint32_t baudrate;
	uint32_t rx_timeout;
	uint32_t tx_timeout;
} lpf_can_config_t;

typedef struct {
	uint32_t can_id;
	uint8_t dlc;
	uint8_t data[LPF_CAN_MAX_DATA_LEN];
	uint32_t timestamp;
} lpf_can_frame_t;

typedef void *lpf_hw_transport_uart_handle_t;
typedef void *lpf_serial_handle_t;

#define LPF_SERIAL_PARITY_NONE 0x00
#define LPF_SERIAL_PARITY_ODD  0x01
#define LPF_SERIAL_PARITY_EVEN 0x02

#define LPF_SERIAL_FLOW_NONE 0x00
#define LPF_SERIAL_FLOW_HW   0x01
#define LPF_SERIAL_FLOW_SW   0x02

typedef struct {
	uint32_t baud_rate;
	uint8_t data_bits;
	uint8_t stop_bits;
	uint8_t parity;
	uint8_t flow_control;
} lpf_serial_config_t;

#endif /* LPF_HW_TYPES_H */
