/************************************************************************
 * HAL层 - GPIO硬件抽象接口 (macOS模拟实现)
 *
 * macOS没有真实GPIO硬件，提供模拟实现用于测试
 ************************************************************************/

#include "hal_gpio.h"
#include "lib/osal_errno.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

/*===========================================================================
 * GPIO模拟状态
 *===========================================================================*/

#define MAX_GPIO_PINS 256

typedef struct {
    bool initialized;
    hal_gpio_direction_t direction;
    hal_gpio_level_t level;
    hal_gpio_edge_t edge;
    hal_gpio_isr_callback_t callback;
    void *user_data;
    bool interrupt_enabled;
} gpio_state_t;

static gpio_state_t gpio_states[MAX_GPIO_PINS];
static pthread_mutex_t gpio_mutex = PTHREAD_MUTEX_INITIALIZER;

/*===========================================================================
 * GPIO API实现
 *===========================================================================*/

int32_t HAL_GPIO_Init(uint32_t gpio_num, const hal_gpio_config_t *config)
{
    if (!config || gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);

    gpio_states[gpio_num].initialized = true;
    gpio_states[gpio_num].direction = config->direction;
    gpio_states[gpio_num].level = config->initial_level;
    gpio_states[gpio_num].edge = config->edge;
    gpio_states[gpio_num].callback = config->callback;
    gpio_states[gpio_num].user_data = config->user_data;
    gpio_states[gpio_num].interrupt_enabled = (config->edge != HAL_GPIO_EDGE_NONE);

    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_Deinit(uint32_t gpio_num)
{
    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);
    memset(&gpio_states[gpio_num], 0, sizeof(gpio_state_t));
    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_SetDirection(uint32_t gpio_num, hal_gpio_direction_t direction)
{
    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);
    if (!gpio_states[gpio_num].initialized) {
        pthread_mutex_unlock(&gpio_mutex);
        return OSAL_EINVAL;
    }
    gpio_states[gpio_num].direction = direction;
    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_GetDirection(uint32_t gpio_num, hal_gpio_direction_t *direction)
{
    if (gpio_num >= MAX_GPIO_PINS || !direction) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);
    if (!gpio_states[gpio_num].initialized) {
        pthread_mutex_unlock(&gpio_mutex);
        return OSAL_EINVAL;
    }
    *direction = gpio_states[gpio_num].direction;
    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_SetLevel(uint32_t gpio_num, hal_gpio_level_t level)
{
    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);
    if (!gpio_states[gpio_num].initialized) {
        pthread_mutex_unlock(&gpio_mutex);
        return OSAL_EINVAL;
    }
    if (gpio_states[gpio_num].direction != HAL_GPIO_DIR_OUTPUT) {
        pthread_mutex_unlock(&gpio_mutex);
        return OSAL_EINVAL;
    }
    gpio_states[gpio_num].level = level;
    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_GetLevel(uint32_t gpio_num, hal_gpio_level_t *level)
{
    if (gpio_num >= MAX_GPIO_PINS || !level) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);
    if (!gpio_states[gpio_num].initialized) {
        pthread_mutex_unlock(&gpio_mutex);
        return OSAL_EINVAL;
    }
    *level = gpio_states[gpio_num].level;
    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_SetInterrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
                               hal_gpio_isr_callback_t callback, void *user_data)
{
    if (gpio_num >= MAX_GPIO_PINS || !callback) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);
    if (!gpio_states[gpio_num].initialized) {
        pthread_mutex_unlock(&gpio_mutex);
        return OSAL_EINVAL;
    }

    gpio_states[gpio_num].edge = edge;
    gpio_states[gpio_num].callback = callback;
    gpio_states[gpio_num].user_data = user_data;
    gpio_states[gpio_num].interrupt_enabled = (edge != HAL_GPIO_EDGE_NONE);

    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_EnableInterrupt(uint32_t gpio_num)
{
    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);
    if (!gpio_states[gpio_num].initialized) {
        pthread_mutex_unlock(&gpio_mutex);
        return OSAL_EINVAL;
    }
    gpio_states[gpio_num].interrupt_enabled = true;
    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}

int32_t HAL_GPIO_DisableInterrupt(uint32_t gpio_num)
{
    if (gpio_num >= MAX_GPIO_PINS) {
        return OSAL_EINVAL;
    }

    pthread_mutex_lock(&gpio_mutex);
    if (!gpio_states[gpio_num].initialized) {
        pthread_mutex_unlock(&gpio_mutex);
        return OSAL_EINVAL;
    }
    gpio_states[gpio_num].interrupt_enabled = false;
    pthread_mutex_unlock(&gpio_mutex);

    return OSAL_SUCCESS;
}
