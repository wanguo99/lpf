/************************************************************************
 * HAL层 - GPIO驱动 (macOS 打桩实现)
 *
 * 说明：
 * - 这是 macOS 平台的打桩实现，仅用于编译
 * - 所有函数返回 OSAL_ERR_NOT_IMPLEMENTED
 * - 实际硬件访问需要在 Linux 平台上运行
 ************************************************************************/

#include "hal.h"
#include "osal.h"

int32_t HAL_GPIO_init(uint32_t gpio_num, const hal_gpio_config_t *config)
{
    (void)gpio_num;
    (void)config;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_deinit(uint32_t gpio_num)
{
    (void)gpio_num;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_set_direction(uint32_t gpio_num, hal_gpio_direction_t direction)
{
    (void)gpio_num;
    (void)direction;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_get_direction(uint32_t gpio_num, hal_gpio_direction_t *direction)
{
    (void)gpio_num;
    (void)direction;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_set_level(uint32_t gpio_num, hal_gpio_level_t level)
{
    (void)gpio_num;
    (void)level;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_get_level(uint32_t gpio_num, hal_gpio_level_t *level)
{
    (void)gpio_num;
    (void)level;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_set_interrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
                               hal_gpio_isr_callback_t callback, void *user_data)
{
    (void)gpio_num;
    (void)edge;
    (void)callback;
    (void)user_data;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_enable_interrupt(uint32_t gpio_num)
{
    (void)gpio_num;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_disable_interrupt(uint32_t gpio_num)
{
    (void)gpio_num;
    return OSAL_ERR_NOT_IMPLEMENTED;
}
