/************************************************************************
 * HAL层 - GPIO驱动 (macOS 打桩实现)
 *
 * 说明：
 * - 这是 macOS 平台的打桩实现，仅用于编译
 * - 所有函数返回 OSAL_ERR_NOT_IMPLEMENTED
 * - 实际硬件访问需要在 Linux 平台上运行
 ************************************************************************/

#include "hal/hal_gpio_api.h"
#include "osal.h"

int32_t HAL_GPIO_Init(uint32_t gpio_num, const hal_gpio_config_t *config)
{
    (void)gpio_num;
    (void)config;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_Deinit(uint32_t gpio_num)
{
    (void)gpio_num;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_SetDirection(uint32_t gpio_num, hal_gpio_direction_t direction)
{
    (void)gpio_num;
    (void)direction;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_GetDirection(uint32_t gpio_num, hal_gpio_direction_t *direction)
{
    (void)gpio_num;
    (void)direction;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_SetLevel(uint32_t gpio_num, hal_gpio_level_t level)
{
    (void)gpio_num;
    (void)level;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_GetLevel(uint32_t gpio_num, hal_gpio_level_t *level)
{
    (void)gpio_num;
    (void)level;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_SetInterrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
                               hal_gpio_isr_callback_t callback, void *user_data)
{
    (void)gpio_num;
    (void)edge;
    (void)callback;
    (void)user_data;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_EnableInterrupt(uint32_t gpio_num)
{
    (void)gpio_num;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_GPIO_DisableInterrupt(uint32_t gpio_num)
{
    (void)gpio_num;
    return OSAL_ERR_NOT_IMPLEMENTED;
}
