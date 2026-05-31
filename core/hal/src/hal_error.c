/************************************************************************
 * HAL层 - 错误码实现
 ************************************************************************/

#include "hal_error.h"
#include "osal.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/************************************************************************
 * 线程局部错误上下文
 ************************************************************************/

static _Thread_local hal_error_context_t g_hal_error_ctx = {0};

/************************************************************************
 * 错误上下文管理
 ************************************************************************/

void HAL_SetErrorContext(int32_t hal_err, int32_t sys_err, const char *fmt, ...)
{
    va_list args;

    g_hal_error_ctx.hal_error = hal_err;
    g_hal_error_ctx.sys_errno = sys_err;

    if (fmt != NULL) {
        va_start(args, fmt);
        vsnprintf(g_hal_error_ctx.message, HAL_ERROR_MSG_SIZE, fmt, args);
        va_end(args);
        g_hal_error_ctx.message[HAL_ERROR_MSG_SIZE - 1] = '\0';
    } else {
        g_hal_error_ctx.message[0] = '\0';
    }
}

const hal_error_context_t* HAL_GetLastError(void)
{
    return &g_hal_error_ctx;
}

void HAL_ClearError(void)
{
    memset(&g_hal_error_ctx, 0, sizeof(hal_error_context_t));
}

/************************************************************************
 * errno到HAL错误码映射
 ************************************************************************/

int32_t HAL_ErrnoToError(int32_t sys_errno)
{
    switch (sys_errno) {
        case 0:
            return HAL_SUCCESS;

        /* 参数错误 */
        case EINVAL:
            return HAL_ERR_INVALID_PARAM;
        case EFAULT:
            return HAL_ERR_INVALID_POINTER;
        case EBADF:
        case EBADFD:
            return HAL_ERR_INVALID_ID;

        /* 资源错误 */
        case ENOMEM:
            return HAL_ERR_NO_MEMORY;
        case EBUSY:
        case ETXTBSY:
            return HAL_ERR_BUSY;
        case ETIMEDOUT:
            return HAL_ERR_TIMEOUT;

        /* 权限错误 */
        case EACCES:
        case EPERM:
            return HAL_ERR_PERMISSION;

        /* 设备错误 */
        case ENODEV:
        case ENXIO:
            return HAL_ERR_NO_DEVICE;
        case ENOENT:
            return HAL_ERR_DEVICE_NOT_FOUND;
        case ENAMETOOLONG:
            return HAL_ERR_NAME_TOO_LONG;

        /* I/O错误 */
        case EIO:
        case EREMOTEIO:
            return HAL_ERR_IO;
        case EPROTO:
        case EPROTOTYPE:
            return HAL_ERR_COMM_FAILED;

        /* 不支持 */
        case ENOSYS:
#if defined(ENOTSUP) && (!defined(EOPNOTSUPP) || ENOTSUP != EOPNOTSUPP)
        case ENOTSUP:
#endif
#ifdef EOPNOTSUPP
        case EOPNOTSUPP:
#endif
            return HAL_ERR_NOT_SUPPORTED;

        /* 其他错误 */
        default:
            return HAL_ERR_GENERIC;
    }
}
