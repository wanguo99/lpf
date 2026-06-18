/************************************************************************
 * HAL层 - GPIO硬件抽象层API
 *
 * 提供统一的GPIO访问接口（基于Linux sysfs GPIO）
 ************************************************************************/

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * GPIO方向定义
 *===========================================================================*/

typedef enum {
	HAL_GPIO_DIR_INPUT = 0x00, /* 输入模式 */
	HAL_GPIO_DIR_OUTPUT = 0x01 /* 输出模式 */
} hal_gpio_direction_t;

/*===========================================================================
 * GPIO电平定义
 *===========================================================================*/

typedef enum {
	HAL_GPIO_LEVEL_LOW = 0x00, /* 低电平 */
	HAL_GPIO_LEVEL_HIGH = 0x01 /* 高电平 */
} hal_gpio_level_t;

/*===========================================================================
 * GPIO中断触发模式
 *===========================================================================*/

typedef enum {
	HAL_GPIO_EDGE_NONE = 0x00, /* 无中断 */
	HAL_GPIO_EDGE_RISING = 0x01, /* 上升沿触发 */
	HAL_GPIO_EDGE_FALLING = 0x02, /* 下降沿触发 */
	HAL_GPIO_EDGE_BOTH = 0x03 /* 双边沿触发 */
} hal_gpio_edge_t;

/*===========================================================================
 * GPIO中断回调函数类型
 *===========================================================================*/

/**
 * @brief GPIO中断回调函数
 *
 * @param gpio_num  触发中断的GPIO引脚号
 * @param level     当前电平
 * @param user_data 用户数据指针
 */
typedef void (*hal_gpio_isr_callback_t)(uint32_t gpio_num,
										hal_gpio_level_t level,
										void *user_data);

/*===========================================================================
 * GPIO配置结构
 *===========================================================================*/

typedef struct {
	hal_gpio_direction_t direction; /* GPIO方向 */
	hal_gpio_level_t initial_level; /* 初始电平（仅输出模式有效） */
	hal_gpio_edge_t edge; /* 中断触发模式 */
	hal_gpio_isr_callback_t callback; /* 中断回调函数 */
	void *user_data; /* 用户数据指针 */
} hal_gpio_config_t;

/*===========================================================================
 * API函数
 *===========================================================================*/

/**
 * @brief 初始化GPIO引脚
 *
 * 导出GPIO引脚并配置方向、初始电平和中断。
 *
 * @param[in] gpio_num GPIO引脚号（芯片特定）
 * @param[in] config   GPIO配置参数
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_PERMISSION    权限不足
 * @return OSAL_EIO               I/O错误
 *
 * @note 如果GPIO已被导出，会先释放再重新初始化
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_init(uint32_t gpio_num, const hal_gpio_config_t *config);

/**
 * @brief 释放GPIO引脚
 *
 * 取消导出GPIO引脚，释放资源。
 *
 * @param[in] gpio_num GPIO引脚号
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO               I/O错误
 *
 * @note 如果配置了中断，会自动停止中断监听线程
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_deinit(uint32_t gpio_num);

/**
 * @brief 设置GPIO方向
 *
 * 配置GPIO引脚为输入或输出模式。
 *
 * @param[in] gpio_num  GPIO引脚号
 * @param[in] direction GPIO方向
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO               I/O错误
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_set_direction(uint32_t gpio_num,
							   hal_gpio_direction_t direction);

/**
 * @brief 获取GPIO方向
 *
 * 读取GPIO引脚的当前方向。
 *
 * @param[in]  gpio_num  GPIO引脚号
 * @param[out] direction 返回的GPIO方向
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO               I/O错误
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_get_direction(uint32_t gpio_num,
							   hal_gpio_direction_t *direction);

/**
 * @brief 设置GPIO输出电平
 *
 * 设置GPIO引脚的输出电平（仅输出模式有效）。
 *
 * @param[in] gpio_num GPIO引脚号
 * @param[in] level    输出电平
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO               I/O错误
 *
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_set_level(uint32_t gpio_num, hal_gpio_level_t level);

/**
 * @brief 读取GPIO输入电平
 *
 * 读取GPIO引脚的当前电平。
 *
 * @param[in]  gpio_num GPIO引脚号
 * @param[out] level    返回的当前电平
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO               I/O错误
 *
 * @note 输入和输出模式都可以读取电平
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_get_level(uint32_t gpio_num, hal_gpio_level_t *level);

/**
 * @brief 配置GPIO中断
 *
 * 设置GPIO引脚的中断触发模式和回调函数。
 *
 * @param[in] gpio_num  GPIO引脚号
 * @param[in] edge      中断触发模式
 * @param[in] callback  中断回调函数（可为NULL以禁用中断）
 * @param[in] user_data 传递给回调函数的用户数据
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NOT_SUPPORTED 平台不支持GPIO中断
 * @return OSAL_EIO               I/O错误
 *
 * @note 回调函数在独立的中断监听线程中执行，注意线程安全
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_set_interrupt(uint32_t gpio_num, hal_gpio_edge_t edge,
							   hal_gpio_isr_callback_t callback,
							   void *user_data);

/**
 * @brief 使能GPIO中断
 *
 * 启动中断监听线程，开始响应GPIO中断。
 *
 * @param[in] gpio_num GPIO引脚号
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效（未配置中断）
 * @return OSAL_EIO               I/O错误
 *
 * @note 必须先调用HAL_GPIO_SetInterrupt配置中断
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_enable_interrupt(uint32_t gpio_num);

/**
 * @brief 禁用GPIO中断
 *
 * 停止中断监听线程，停止响应GPIO中断。
 *
 * @param[in] gpio_num GPIO引脚号
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_EIO               I/O错误
 *
 * @note 中断配置仍然保留，可以重新使能
 * @note 线程安全：使用文件锁保护多进程并发访问
 */
int32_t hal_gpio_disable_interrupt(uint32_t gpio_num);

#ifdef __cplusplus
}
#endif

#endif /* HAL_GPIO_H */
