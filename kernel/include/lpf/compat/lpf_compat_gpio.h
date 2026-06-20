// SPDX-License-Identifier: GPL-2.0

#ifndef LPF_COMPAT_GPIO_H
#define LPF_COMPAT_GPIO_H

#include "lpf/types/lpf_gpio_types.h"

int32_t lpf_compat_gpio_request(uint32_t gpio_num, const char *label);
void lpf_compat_gpio_free(uint32_t gpio_num);
int32_t lpf_compat_gpio_set_direction(uint32_t gpio_num,
				      lpf_gpio_direction_t direction,
				      lpf_gpio_level_t initial_level);
int32_t lpf_compat_gpio_set_level(uint32_t gpio_num,
				  lpf_gpio_level_t level);
int32_t lpf_compat_gpio_get_level(uint32_t gpio_num,
				  lpf_gpio_level_t *level);
int32_t lpf_compat_gpio_request_interrupt(
	uint32_t gpio_num, lpf_gpio_edge_t edge,
	lpf_gpio_irq_callback_t callback, void *user_data,
	void **irq_handle);
void lpf_compat_gpio_free_interrupt(void *irq_handle);
int32_t lpf_compat_gpio_enable_interrupt(void *irq_handle);
int32_t lpf_compat_gpio_disable_interrupt(void *irq_handle);

#endif /* LPF_COMPAT_GPIO_H */
