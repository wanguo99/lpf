// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_HW_GPIO_H
#define LPF_HW_GPIO_H

#include "lpf/lpf_hw_types.h"

int32_t lpf_hw_gpio_init(uint32_t gpio_num,
			 const lpf_gpio_config_t *config);
int32_t lpf_hw_gpio_deinit(uint32_t gpio_num);
int32_t lpf_hw_gpio_set_direction(uint32_t gpio_num,
				  lpf_gpio_direction_t direction);
int32_t lpf_hw_gpio_get_direction(uint32_t gpio_num,
				  lpf_gpio_direction_t *direction);
int32_t lpf_hw_gpio_set_level(uint32_t gpio_num, lpf_gpio_level_t level);
int32_t lpf_hw_gpio_get_level(uint32_t gpio_num, lpf_gpio_level_t *level);
int32_t lpf_hw_gpio_set_interrupt(uint32_t gpio_num, lpf_gpio_edge_t edge,
				  lpf_gpio_irq_callback_t callback,
				  void *user_data);
int32_t lpf_hw_gpio_enable_interrupt(uint32_t gpio_num);
int32_t lpf_hw_gpio_disable_interrupt(uint32_t gpio_num);

#endif /* LPF_HW_GPIO_H */
