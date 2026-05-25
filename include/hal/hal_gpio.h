/************************************************************************
 * HAL层 - GPIO硬件抽象接口
 *
 * 功能：
 * - GPIO输入/输出控制
 * - GPIO中断支持
 * - 跨平台抽象（Linux sysfs GPIO / 模拟实现）
 ************************************************************************/

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "osal.h"

/*===========================================================================
 * GPIO方向定义
 *===========================================================================*/

typedef enum {
    HAL_GPIO_DIR_INPUT  = 0,    /* 输入模式 */
    HAL_GPIO_DIR_OUTPUT = 1     /* 输出模式 */
} hal_gpio_direction_t;

/*===========================================================================
 * GPIO电平定义
 *===========================================================================*/

typedef enum {
    HAL_GPIO_LEVEL_LOW  = 0,    /* 低电平 */
    HAL_GPIO_LEVEL_HIGH = 1     /* 高电平 */
} hal_gpio_level_t;

/*===========================================================================
 * GPIO中断触发模式
 *===========================================================================*/

typedef enum {
    HAL_GPIO_EDGE_NONE    = 0,  /* 无中断 */
    HAL_GPIO_EDGE_RISING  = 1,  /* 上升沿触发 */
    HAL_GPIO_EDGE_FALLING = 2,  /* 下降沿触发 */
    HAL_GPIO_EDGE_BOTH    = 3   /* 双边沿触发 */
} hal_gpio_edge_t;

/*===========================================================================
 * GPIO中断回调函数类型
 *===========================================================================*/

/**
 * @brief GPIO中断回调函数
 * @param gpio_num GPIO引脚号
 * @param level 当前电平
 * @param user_data 用户数据
 */
typedef void (*hal_gpio_isr_callback_t)(uint32_t gpio_num, hal_gpio_level_t level, void *user_data);

/*===========================================================================
 * GPIO配置结构
 *===========================================================================*/

typedef struct {
    hal_gpio_direction_t direction;     /* GPIO方向 */
    hal_gpio_level_t initial_level;     /* 初始电平（输出模式） */
    hal_gpio_edge_t edge;               /* 中断触发模式 */
    hal_gpio_isr_callback_t callback;   /* 中断回调函数 */
    void *user_data;                    /* 用户数据 */
} hal_gpio_config_t;

/*===========================================================================
 * GPIO API接口
 *===========================================================================*/

/**
 * @brief 初始化GPIO引脚
 *
 * @param gpio_num GPIO引脚号
 * @param config GPIO配置
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_PERMISSION 权限不足
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_Init(uint32_t gpio_num, const hal_gpio_config_t *config);

/**
 * @brief 释放GPIO引脚
 *
 * @param gpio_num GPIO引脚号
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_Deinit(uint32_t gpio_num);

/**
 * @brief 设置GPIO方向
 *
 * @param gpio_num GPIO引脚号
 * @param direction GPIO方向
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_SetDirection(uint32_t gpio_num, hal_gpio_direction_t direction);

/**
 * @brief 获取GPIO方向
 *
 * @param gpio_num GPIO引脚号
 * @param direction 输出：GPIO方向
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_GetDirection(uint32_t gpio_num, hal_gpio_direction_t *direction);

/**
 * @brief 设置GPIO输出电平
 *
 * @param gpio_num GPIO引脚号
 * @param level 输出电平
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_SetLevel(uint32_t gpio_num, hal_gpio_level_t level);

/**
 * @brief 读取GPIO输入电平
 *
 * @param gpio_num GPIO引脚号
 * @param level 输出：当前电平
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_GetLevel(uint32_t gpio_num, hal_gpio_level_t *level);

/**
 * @brief 配置GPIO中断
 *
 * @param gpio_num GPIO引脚号
 * @param edge 中断触发模式
 * @param callback 中断回调函数
 * @param user_data 用户数据
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NOT_SUPPORTED 不支持中断
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_SetInterrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
                               hal_gpio_isr_callback_t callback, void *user_data);

/**
 * @brief 使能GPIO中断
 *
 * @param gpio_num GPIO引脚号
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_EnableInterrupt(uint32_t gpio_num);

/**
 * @brief 禁用GPIO中断
 *
 * @param gpio_num GPIO引脚号
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO I/O错误
 */
int32_t HAL_GPIO_DisableInterrupt(uint32_t gpio_num);

#endif /* HAL_GPIO_H */
