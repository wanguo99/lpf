/************************************************************************
 * Watchdog外设驱动接口
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

#include "osal_types.h"
#include "pdl_types.h"  /* 使用 types 模块的配置类型定义 */

/*
 * Watchdog服务句柄
 */
typedef void* watchdog_handle_t;

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
} watchdog_status_t;

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
int32_t PDL_WATCHDOG_Init(const pdl_watchdog_config_t *config, watchdog_handle_t *handle);

/**
 * @brief 关闭Watchdog服务
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 */
int32_t PDL_WATCHDOG_Deinit(watchdog_handle_t handle);

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
int32_t PDL_WATCHDOG_Start(watchdog_handle_t handle);

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
int32_t PDL_WATCHDOG_Stop(watchdog_handle_t handle);

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
int32_t PDL_WATCHDOG_Kick(watchdog_handle_t handle);

/**
 * @brief 获取Watchdog状态
 *
 * @param[in] handle Watchdog句柄
 * @param[out] status 返回的状态信息
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 */
int32_t PDL_WATCHDOG_GetStatus(watchdog_handle_t handle, watchdog_status_t *status);

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
int32_t PDL_WATCHDOG_SetInterval(watchdog_handle_t handle, uint32_t interval_ms);

/**
 * @brief 启用看门狗
 *
 * @param[in] handle Watchdog句柄
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER 参数为NULL
 * @return OSAL_ERR_GENERIC 失败
 */
int32_t PDL_WATCHDOG_Enable(watchdog_handle_t handle);

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
int32_t PDL_WATCHDOG_Disable(watchdog_handle_t handle);

#endif /* PDL_WATCHDOG_H */
