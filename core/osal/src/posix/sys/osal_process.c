/************************************************************************
 * OSAL - 进程管理接口实现（POSIX）
 ************************************************************************/

#include "osal.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/*
 * 基础进程控制函数（保持兼容）
 */

void OSAL_exit(int32_t status)
{
    exit(status);
}

int32_t OSAL_getpid(void)
{
    return getpid();
}

int32_t OSAL_getppid(void)
{
    return getppid();
}

void OSAL_abort(void)
{
    abort();
}

int32_t OSAL_fork(void)
{
    osal_pid_t pid = fork();
    return (pid < 0) ? -1 : (int32_t)pid;
}

int32_t OSAL_execvp(const char *file, char *const argv[])
{
    return execvp(file, argv);
}

int32_t OSAL_waitpid(int32_t pid, int32_t *status, int32_t options)
{
    int wait_status;
    int posix_options = 0;
    osal_pid_t result;

    /* 转换选项 */
    if (options & OSAL_WNOHANG) {
        posix_options |= WNOHANG;
    }

    result = waitpid((osal_pid_t)pid, &wait_status, posix_options);

    if (result > 0) {
        /* 子进程退出 */
        if (status != NULL) {
            *status = WIFEXITED(wait_status) ? WEXITSTATUS(wait_status) : -1;
        }
        return (int32_t)result;
    }

    if (result == 0) {
        /* 非阻塞模式下子进程未退出 */
        return 0;
    }

    /* result < 0: 错误 */
    return -1;
}

/*
 * 以下为过时的高层封装，将在后续版本中移除
 */
