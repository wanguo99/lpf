/************************************************************************
 * HAL层 - 错误处理API
 *
 * 提供统一的HAL错误码体系和errno映射机制
 ************************************************************************/

#ifndef HAL_ERROR_API_H
#define HAL_ERROR_API_H

#include <stdint.h>
#include "lib/osal_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * HAL错误码定义
 *===========================================================================*/

/* 成功 */
#define HAL_SUCCESS             0

/* 通用错误 */
#define HAL_ERR_GENERIC         -1
#define HAL_ERR_INVALID_PARAM   -2
#define HAL_ERR_INVALID_POINTER -3
#define HAL_ERR_INVALID_ID      -4
#define HAL_ERR_NO_MEMORY       -5
#define HAL_ERR_TIMEOUT         -6
#define HAL_ERR_BUSY            -7
#define HAL_ERR_NOT_SUPPORTED   -8
#define HAL_ERR_PERMISSION      -9
#define HAL_ERR_NAME_TOO_LONG   -10

/* 设备相关错误 */
#define HAL_ERR_NO_DEVICE       -20
#define HAL_ERR_DEVICE_NOT_FOUND -21
#define HAL_ERR_DEVICE_BUSY     -22
#define HAL_ERR_DEVICE_ERROR    -23

/* I/O错误 */
#define HAL_ERR_IO              -30
#define HAL_ERR_READ_FAILED     -31
#define HAL_ERR_WRITE_FAILED    -32
#define HAL_ERR_PARTIAL_IO      -33

/* 通信错误 */
#define HAL_ERR_COMM_FAILED     -40
#define HAL_ERR_NO_RESPONSE     -41
#define HAL_ERR_CHECKSUM        -42

/*===========================================================================
 * 错误上下文结构
 *===========================================================================*/

#define HAL_ERROR_MSG_SIZE 256

/**
 * @brief HAL错误上下文
 *
 * 存储详细的错误信息，包括HAL错误码、系统errno和错误消息。
 */
typedef struct {
	int32_t hal_error;  /* HAL错误码 */
	int32_t sys_errno;  /* 系统errno */
	char message[HAL_ERROR_MSG_SIZE];  /* 错误消息 */
} hal_error_context_t;

/*===========================================================================
 * API函数
 *===========================================================================*/

/**
 * @brief 设置HAL错误上下文
 *
 * 记录HAL错误码、系统errno和格式化的错误消息到线程局部存储。
 *
 * @param[in] hal_err HAL错误码
 * @param[in] sys_err 系统errno（如果不相关则传0）
 * @param[in] fmt     格式化字符串（printf风格）
 * @param[in] ...     可变参数
 *
 * @note 错误上下文存储在线程局部存储中，每个线程独立
 * @note 调用此函数会覆盖该线程之前的错误上下文
 *
 * @example
 *   HAL_SetErrorContext(HAL_ERR_NO_DEVICE, errno, "Failed to open %s", device);
 */
void HAL_SetErrorContext(int32_t hal_err, int32_t sys_err, const char *fmt, ...);

/**
 * @brief 获取最后的HAL错误上下文
 *
 * 返回当前线程最后一次设置的错误上下文。
 *
 * @return 错误上下文指针（线程局部存储）
 *         如果没有错误，返回的上下文中hal_error为HAL_SUCCESS
 *
 * @note 返回的指针指向线程局部存储，不需要释放
 * @note 指针在下次调用HAL_SetErrorContext或HAL_ClearError时失效
 *
 * @example
 *   const hal_error_context_t *err = HAL_GetLastError();
 *   if (err->hal_error != HAL_SUCCESS) {
 *       printf("Error: %s (errno=%d)\n", err->message, err->sys_errno);
 *   }
 */
const hal_error_context_t* HAL_GetLastError(void);

/**
 * @brief 清除错误上下文
 *
 * 重置当前线程的错误上下文为无错误状态。
 *
 * @note 清除后，HAL_GetLastError返回的hal_error为HAL_SUCCESS
 */
void HAL_ClearError(void);

/**
 * @brief 将系统errno映射为HAL错误码
 *
 * 将POSIX errno值转换为对应的HAL错误码。
 *
 * @param[in] sys_errno 系统errno值
 *
 * @return 对应的HAL错误码
 *
 * @note 映射规则：
 *   - EINVAL     -> HAL_ERR_INVALID_PARAM
 *   - ENOMEM     -> HAL_ERR_NO_MEMORY
 *   - ETIMEDOUT  -> HAL_ERR_TIMEOUT
 *   - EBUSY      -> HAL_ERR_BUSY
 *   - ENOTSUP    -> HAL_ERR_NOT_SUPPORTED
 *   - EACCES     -> HAL_ERR_PERMISSION
 *   - ENODEV     -> HAL_ERR_NO_DEVICE
 *   - EIO        -> HAL_ERR_IO
 *   - 其他       -> HAL_ERR_GENERIC
 */
int32_t HAL_ErrnoToError(int32_t sys_errno);

/*===========================================================================
 * 便捷宏
 *===========================================================================*/

/**
 * @brief 设置错误并记录上下文
 *
 * 便捷宏，用于快速设置错误上下文。
 *
 * @example
 *   if (fd < 0) {
 *       HAL_SET_ERROR(HAL_ERR_NO_DEVICE, errno, "Failed to open %s", device);
 *       return HAL_ERR_NO_DEVICE;
 *   }
 */
#define HAL_SET_ERROR(hal_err, sys_err, ...) \
	HAL_SetErrorContext(hal_err, sys_err, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* HAL_ERROR_API_H */
