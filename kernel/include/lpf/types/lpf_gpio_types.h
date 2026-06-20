// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_GPIO_TYPES_H
#define LPF_GPIO_TYPES_H

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

#endif /* LPF_GPIO_TYPES_H */
