/************************************************************************
 * 信号处理实现 (Linux)
 ************************************************************************/

#include "osal.h"
#include <signal.h>
#include <string.h>
#include <errno.h>

int32_t OSAL_SignalRegister(int32_t signum, os_signal_handler_t handler)
{
    struct sigaction sa;

    if (NULL == handler)
        return OSAL_ERR_INVALID_POINTER;

    memset(&sa, 0, sizeof(sa));

    union {
        os_signal_handler_t osal_handler;
        void (*posix_handler)(int);
    } handler_union;

    handler_union.osal_handler = handler;
    sa.sa_handler = handler_union.posix_handler;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(signum, &sa, NULL) < 0)
    {
        OSAL_Printf("[OSAL] sigaction失败, signum=%d, errno=%d\n", signum, errno);
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

int32_t OSAL_SignalIgnore(int32_t signum)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(signum, &sa, NULL) < 0)
    {
        OSAL_Printf("[OSAL] sigaction(SIG_IGN)失败, signum=%d, errno=%d\n", signum, errno);
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

int32_t OSAL_SignalDefault(int32_t signum)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(signum, &sa, NULL) < 0)
    {
        OSAL_Printf("[OSAL] sigaction(SIG_DFL)失败, signum=%d, errno=%d\n", signum, errno);
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

int32_t OSAL_SignalBlock(int32_t signum)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, signum);

    if (sigprocmask(SIG_BLOCK, &set, NULL) < 0)
    {
        OSAL_Printf("[OSAL] sigprocmask(SIG_BLOCK)失败, signum=%d, errno=%d\n", signum, errno);
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

int32_t OSAL_SignalUnblock(int32_t signum)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, signum);

    if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0)
    {
        OSAL_Printf("[OSAL] sigprocmask(SIG_UNBLOCK)失败, signum=%d, errno=%d\n", signum, errno);
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}
