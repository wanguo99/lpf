/************************************************************************
 * OSAL - errno系统调用封装实现（POSIX）
 ************************************************************************/

#include <osal.h>
#include <errno.h>
#include <string.h>

/*===========================================================================
 * errno访问函数实现
 *===========================================================================*/

int32_t OSAL_GetErrno(void)
{
    return errno;
}

void OSAL_SetErrno(int32_t err)
{
    errno = err;
}

const char *OSAL_StrError(int32_t errnum)
{
    return strerror(errnum);
}

/*===========================================================================
 * OSAL状态码转字符串实现
 * 注意：多个OSAL错误码可能映射到同一个errno值，只保留第一个
 *===========================================================================*/

const char *OSAL_GetStatusName(int32_t status_code)
{
    switch (status_code)
    {
        case OSAL_SUCCESS:                return "OSAL_SUCCESS";

        /* errno 映射的错误码 - 只保留每个errno值的第一个别名 */
        case OSAL_ERR_GENERIC:            return "OSAL_ERR_GENERIC";  /* EIO=5 */
        case OSAL_ERR_INVALID_POINTER:    return "OSAL_ERR_INVALID_POINTER";  /* EFAULT=14 */
        case OSAL_ERR_NO_MEMORY:          return "OSAL_ERR_NO_MEMORY";  /* ENOMEM=12 */
        case OSAL_ERR_INVALID_SIZE:       return "OSAL_ERR_INVALID_SIZE";  /* EINVAL=22 */
        case OSAL_ERR_NAME_TOO_LONG:      return "OSAL_ERR_NAME_TOO_LONG";  /* ENAMETOOLONG=63 */
        case OSAL_ERR_NAME_TAKEN:         return "OSAL_ERR_NAME_TAKEN";  /* EEXIST=17 */
        case OSAL_ERR_NAME_NOT_FOUND:     return "OSAL_ERR_NAME_NOT_FOUND";  /* ENOENT=2 */
        case OSAL_ERR_TIMEOUT:            return "OSAL_ERR_TIMEOUT";  /* ETIMEDOUT=60/110 */
        case OSAL_ERR_NOT_IMPLEMENTED:    return "OSAL_ERR_NOT_IMPLEMENTED";  /* ENOSYS=78 */
        case OSAL_ERR_BUSY:               return "OSAL_ERR_BUSY";  /* EBUSY=16 */
        case OSAL_ERR_PERMISSION:         return "OSAL_ERR_PERMISSION";  /* EPERM=1 */
        case OSAL_ERR_NOT_SUPPORTED:      return "OSAL_ERR_NOT_SUPPORTED";  /* ENOTSUP=45 */
        case OSAL_ERR_WOULD_BLOCK:        return "OSAL_ERR_WOULD_BLOCK";  /* EWOULDBLOCK=EAGAIN=35 */
        case OSAL_ERR_INTERRUPTED:        return "OSAL_ERR_INTERRUPTED";  /* EINTR=4 */
        case OSAL_ERR_RESOURCE_LIMIT:     return "OSAL_ERR_RESOURCE_LIMIT";  /* EMFILE=24 */
        /* OSAL_ERR_TIMER_UNAVAILABLE 也映射到 EAGAIN=35，与 EWOULDBLOCK 冲突，已移除 */

        /* OSAL 特定错误码 (200+) */
        case OSAL_ERR_ADDRESS_MISALIGNED: return "OSAL_ERR_ADDRESS_MISALIGNED";
        case OSAL_ERR_INVALID_INT_NUM:    return "OSAL_ERR_INVALID_INT_NUM";
        case OSAL_ERR_INVALID_PRIORITY:   return "OSAL_ERR_INVALID_PRIORITY";
        case OSAL_ERR_INVALID_STATE:      return "OSAL_ERR_INVALID_STATE";
        case OSAL_ERR_NO_FREE_IDS:        return "OSAL_ERR_NO_FREE_IDS";
        case OSAL_ERR_SEM_FAILURE:        return "OSAL_ERR_SEM_FAILURE";
        case OSAL_ERR_SEM_NOT_FULL:       return "OSAL_ERR_SEM_NOT_FULL";
        case OSAL_ERR_INVALID_SEM_VALUE:  return "OSAL_ERR_INVALID_SEM_VALUE";
        case OSAL_ERR_QUEUE_EMPTY:        return "OSAL_ERR_QUEUE_EMPTY";
        case OSAL_ERR_QUEUE_FULL:         return "OSAL_ERR_QUEUE_FULL";
        case OSAL_ERR_QUEUE_ID:           return "OSAL_ERR_QUEUE_ID";
        case OSAL_ERR_TIMER_INVALID_ARGS: return "OSAL_ERR_TIMER_INVALID_ARGS";
        case OSAL_ERR_TIMER_ID:           return "OSAL_ERR_TIMER_ID";
        case OSAL_ERR_TIMER_INTERNAL:     return "OSAL_ERR_TIMER_INTERNAL";

        default:                          return "UNKNOWN_ERROR";
    }
}

const char *OSAL_StatusToString(osal_status_t status)
{
    if (status == OSAL_SUCCESS) {
        return "Success";
    }

    /* OSAL 特定错误码 (200+) */
    switch (status) {
        case OSAL_ERR_ADDRESS_MISALIGNED:
            return "Address misaligned";
        case OSAL_ERR_INVALID_INT_NUM:
            return "Invalid interrupt number";
        case OSAL_ERR_INVALID_PRIORITY:
            return "Invalid priority";
        case OSAL_ERR_INVALID_STATE:
            return "Invalid state";
        case OSAL_ERR_NO_FREE_IDS:
            return "No free IDs available";
        case OSAL_ERR_SEM_FAILURE:
            return "Semaphore operation failed";
        case OSAL_ERR_SEM_NOT_FULL:
            return "Semaphore not full";
        case OSAL_ERR_INVALID_SEM_VALUE:
            return "Invalid semaphore value";
        case OSAL_ERR_QUEUE_EMPTY:
            return "Queue is empty";
        case OSAL_ERR_QUEUE_FULL:
            return "Queue is full";
        case OSAL_ERR_QUEUE_ID:
            return "Invalid queue ID";
        case OSAL_ERR_TIMER_INVALID_ARGS:
            return "Invalid timer arguments";
        case OSAL_ERR_TIMER_ID:
            return "Invalid timer ID";
        case OSAL_ERR_TIMER_INTERNAL:
            return "Timer internal error";
        default:
            /* 对于 errno 映射的错误码，使用系统的 strerror */
            if (status > 0 && status < 200) {
                return strerror(status);
            }
            return "Unknown error";
    }
}
