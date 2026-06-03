/************************************************************************
 * HAL层 - 错误码实现
 ************************************************************************/

#include "hal/hal_error_api.h"
#include "osal/osal.h"
#include "osal.h"
#include <stdarg.h>

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
        OSAL_Vsnprintf(g_hal_error_ctx.message, HAL_ERROR_MSG_SIZE, fmt, args);
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
    OSAL_Memset(&g_hal_error_ctx, 0, sizeof(hal_error_context_t));
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
        case OSAL_EINVAL:
            return HAL_ERR_INVALID_PARAM;
        case OSAL_EFAULT:
            return HAL_ERR_INVALID_POINTER;
        case OSAL_EBADF:
        case OSAL_EBADFD:
            return HAL_ERR_INVALID_ID;

        /* 资源错误 */
        case OSAL_ENOMEM:
            return HAL_ERR_NO_MEMORY;
        case OSAL_EBUSY:
        case OSAL_ETXTBSY:
            return HAL_ERR_BUSY;
        case OSAL_ETIMEDOUT:
            return HAL_ERR_TIMEOUT;

        /* 权限错误 */
        case OSAL_EACCES:
        case OSAL_EPERM:
            return HAL_ERR_PERMISSION;

        /* 设备错误 */
        case OSAL_ENODEV:
        case OSAL_ENXIO:
            return HAL_ERR_NO_DEVICE;
        case OSAL_ENOENT:
            return HAL_ERR_DEVICE_NOT_FOUND;
        case OSAL_ENAMETOOLONG:
            return HAL_ERR_NAME_TOO_LONG;

        /* I/O错误 */
        case OSAL_EIO:
        case OSAL_EREMOTEIO:
            return HAL_ERR_IO;
        case OSAL_EPROTO:
        case OSAL_EPROTOTYPE:
            return HAL_ERR_COMM_FAILED;

        /* 不支持 */
        case OSAL_ENOSYS:
#if defined(OSAL_ENOTSUP) && (!defined(OSAL_EOPNOTSUPP) || OSAL_ENOTSUP != OSAL_EOPNOTSUPP)
        case OSAL_ENOTSUP:
#endif
#ifdef OSAL_EOPNOTSUPP
        case OSAL_EOPNOTSUPP:
#endif
            return HAL_ERR_NOT_SUPPORTED;

        /* 其他错误 */
        default:
            return HAL_ERR_GENERIC;
    }
}
