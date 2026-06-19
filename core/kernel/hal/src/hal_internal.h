// SPDX-License-Identifier: GPL-2.0

#ifndef HAL_INTERNAL_H
#define HAL_INTERNAL_H

#ifdef CONFIG_HAL_GPIO
int hal_gpio_module_init(void);
void hal_gpio_module_deinit(void);
#endif

#endif /* HAL_INTERNAL_H */
