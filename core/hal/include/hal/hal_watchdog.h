/************************************************************************
 * HAL层 - 看门狗硬件抽象层API
 *
 * 提供统一的看门狗访问接口（基于Linux /dev/watchdog）
 ************************************************************************/

#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 类型定义
 *===========================================================================*/

/**
 * @brief 看门狗设备句柄（不透明指针）
 */
typedef void* hal_watchdog_handle_t;

/**
 * @brief 看门狗配置结构
 */
typedef struct
{
	const char *device;      /* 设备路径，如 "/dev/watchdog" */
	uint32_t    timeout_sec; /* 超时时间（秒），0表示使用默认值 */
	bool        enable_on_init; /* 初始化时是否立即启用看门狗 */
} hal_watchdog_config_t;

/*===========================================================================
 * API函数
 *===========================================================================*/

/**
 * @brief 初始化看门狗驱动
 *
 * 打开看门狗设备并配置超时时间。
 *
 * @param[in]  config  看门狗配置参数
 * @param[out] handle  返回的看门狗句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_NO_DEVICE     设备不存在
 * @return OSAL_ERR_GENERIC       打开失败
 *
 * @note 某些硬件看门狗在打开设备时会自动启用，无法停止
 * @note 线程安全：单个看门狗设备通常由单个进程管理
 */
int32_t HAL_WATCHDOG_Init(const hal_watchdog_config_t *config, hal_watchdog_handle_t *handle);

/**
 * @brief 关闭看门狗驱动
 *
 * 释放看门狗资源，关闭设备文件。
 *
 * @param[in] handle 看门狗句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 *
 * @note 关闭看门狗可能导致系统重启，取决于硬件实现
 * @note 在Linux上，写入魔术字符'V'可以安全关闭看门狗（如果硬件支持）
 */
int32_t HAL_WATCHDOG_Deinit(hal_watchdog_handle_t handle);

/**
 * @brief 喂狗（重置看门狗定时器）
 *
 * 重置看门狗计数器，防止系统复位。
 *
 * @param[in] handle 看门狗句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 * @return OSAL_ERR_GENERIC       喂狗失败
 *
 * @note 必须在超时时间内定期调用此函数，否则系统会复位
 */
int32_t HAL_WATCHDOG_Kick(hal_watchdog_handle_t handle);

/**
 * @brief 启用看门狗
 *
 * 启动看门狗定时器。
 *
 * @param[in] handle 看门狗句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 * @return OSAL_ERR_GENERIC       启用失败
 *
 * @note 某些硬件看门狗在Init时就已启用
 */
int32_t HAL_WATCHDOG_Enable(hal_watchdog_handle_t handle);

/**
 * @brief 禁用看门狗
 *
 * 停止看门狗定时器。
 *
 * @param[in] handle 看门狗句柄
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_NOT_SUPPORTED 硬件不支持禁用
 * @return OSAL_ERR_INVALID_PARAM 句柄无效
 * @return OSAL_ERR_GENERIC       禁用失败
 *
 * @note 许多硬件看门狗一旦启用就无法禁用，此时返回OSAL_ERR_NOT_SUPPORTED
 */
int32_t HAL_WATCHDOG_Disable(hal_watchdog_handle_t handle);

/**
 * @brief 设置看门狗超时时间
 *
 * 动态修改看门狗的超时时间。
 *
 * @param[in] handle      看门狗句柄
 * @param[in] timeout_sec 超时时间（秒）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       设置失败
 *
 * @note 实际超时时间可能受硬件限制，驱动会选择最接近的值
 */
int32_t HAL_WATCHDOG_SetTimeout(hal_watchdog_handle_t handle, uint32_t timeout_sec);

/**
 * @brief 获取看门狗超时时间
 *
 * 读取当前配置的超时时间。
 *
 * @param[in]  handle      看门狗句柄
 * @param[out] timeout_sec 返回的超时时间（秒）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       获取失败
 *
 * @note 返回的是实际硬件使用的超时时间，可能与设置的值略有不同
 */
int32_t HAL_WATCHDOG_GetTimeout(hal_watchdog_handle_t handle, uint32_t *timeout_sec);

/**
 * @brief 获取看门狗剩余时间
 *
 * 读取距离系统复位的剩余时间。
 *
 * @param[in]  handle       看门狗句柄
 * @param[out] timeleft_sec 返回的剩余时间（秒）
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_NOT_SUPPORTED 硬件不支持此功能
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 * @return OSAL_ERR_GENERIC       获取失败
 *
 * @note 并非所有看门狗硬件都支持读取剩余时间
 */
int32_t HAL_WATCHDOG_GetTimeleft(hal_watchdog_handle_t handle, uint32_t *timeleft_sec);

/**
 * @brief 获取看门狗统计信息
 *
 * 读取看门狗的运行统计信息。
 *
 * @param[in]  handle     看门狗句柄
 * @param[out] kick_count 返回的喂狗次数
 *
 * @return OSAL_SUCCESS           成功
 * @return OSAL_ERR_INVALID_PARAM 参数无效
 *
 * @note 统计信息由驱动维护，不是硬件功能
 */
int32_t HAL_WATCHDOG_GetStats(hal_watchdog_handle_t handle, uint32_t *kick_count);

#ifdef __cplusplus
}
#endif

#endif /* HAL_WATCHDOG_H */
