/************************************************************************
 * Watchdog外设驱动对外API
 *
 * 功能：
 * - 看门狗设备管理
 * - 自动喂狗服务
 * - 看门狗状态监控
 *
 * 设计理念：
 * - 对外只暴露业务接口（初始化、启动/停止喂狗服务、状态查询）
 * - 内部封装HAL层调用和喂狗线程管理
 * - 支持自动喂狗和手动喂狗两种模式
 ************************************************************************/

#ifndef PDL_WATCHDOG_H
#define PDL_WATCHDOG_H


/*===========================================================================
 * Watchdog 配置类型
 *===========================================================================*/

/**
 * @brief Watchdog工作模式
 */
typedef enum
{
	PDL_WATCHDOG_MODE_MANUAL = 0x00,    /* 手动模式：应用自己调用Kick */
	PDL_WATCHDOG_MODE_AUTO = 0x01       /* 自动模式：PDL内部线程自动喂狗 */
} pdl_watchdog_mode_t;

/**
 * @brief Watchdog配置
 */
typedef struct
{
	char name[0x40];                  /* Watchdog名称 */
	const char *device;             /* 设备路径（如/dev/watchdog） */
	uint32_t timeout_sec;           /* 超时时间（秒） */
	pdl_watchdog_mode_t mode;           /* 工作模式 */
	uint32_t kick_interval_ms;      /* 自动模式下的喂狗间隔（毫秒） */
	bool enable_on_init;            /* 初始化时是否启用看门狗 */
} pdl_watchdog_config_t;

/*
 * Watchdog服务句柄
 */
typedef void* pdl_watchdog_handle_t;

/*
 * Watchdog状态信息
 */
typedef struct
{
	bool enabled;                   /* 是否已启用 */
	bool running;                   /* 自动喂狗线程是否运行中 */
	uint32_t timeout_sec;           /* 超时时间（秒） */
	uint32_t kick_count;            /* 喂狗次数 */
	uint32_t kick_interval_ms;      /* 喂狗间隔（毫秒） */
	pdl_watchdog_mode_t mode;           /* 工作模式 */
	uint64_t last_kick_time_us;     /* 上次喂狗时间戳（微秒） */
} pdl_watchdog_status_t;

/**
 * @brief 初始化Watchdog服务
 *
 * @param[in] config Watchdog配置
 * @param[out] handle 返回的Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_WATCHDOG_init(const pdl_watchdog_config_t *config, pdl_watchdog_handle_t *handle);

/**
 * @brief 关闭Watchdog服务
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 */
int32_t PDL_WATCHDOG_deinit(pdl_watchdog_handle_t handle);

/**
 * @brief 启动自动喂狗服务
 *
 * 仅在自动模式下有效，启动内部喂狗线程
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_WATCHDOG_start(pdl_watchdog_handle_t handle);

/**
 * @brief 停止自动喂狗服务
 *
 * 仅在自动模式下有效，停止内部喂狗线程
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 */
int32_t PDL_WATCHDOG_stop(pdl_watchdog_handle_t handle);

/**
 * @brief 手动喂狗
 *
 * 在手动模式下使用，或在自动模式下额外喂狗
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_WATCHDOG_kick(pdl_watchdog_handle_t handle);

/**
 * @brief 获取Watchdog状态
 *
 * @param[in] handle Watchdog句柄
 * @param[out] status 返回的状态信息
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 */
int32_t PDL_WATCHDOG_get_status(pdl_watchdog_handle_t handle, pdl_watchdog_status_t *status);

/**
 * @brief 设置喂狗间隔
 *
 * 仅在自动模式下有效
 *
 * @param[in] handle Watchdog句柄
 * @param[in] interval_ms 喂狗间隔（毫秒）
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_WATCHDOG_set_interval(pdl_watchdog_handle_t handle, uint32_t interval_ms);

/**
 * @brief 启用看门狗
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_WATCHDOG_enable(pdl_watchdog_handle_t handle);

/**
 * @brief 禁用看门狗
 *
 * 注意：某些硬件看门狗一旦启用就无法禁用
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_NOT_SUPPORTED 硬件不支持禁用
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_WATCHDOG_disable(pdl_watchdog_handle_t handle);

#endif /* PDL_WATCHDOG_H */
