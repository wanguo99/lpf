/************************************************************************
 * OSAL - 进程管理接口实现（POSIX）
 ************************************************************************/

#include "osal/sys/osal_process.h"
#include "osal/osal.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/*
 * 基础进程控制函数（保持兼容）
 */

void OSAL_Exit(int32_t status)
{
    exit(status);
}

int32_t OSAL_Getpid(void)
{
    return getpid();
}

int32_t OSAL_Kill(int32_t pid, int32_t sig)
{
    return kill(pid, sig);
}

void OSAL_Abort(void)
{
    abort();
}

/*
 * 进程管理接口（新增）
 */

int32_t OSAL_ProcessCreate(osal_id_t *proc_id, const char *path,
                         char *const argv[], char *const envp[])
{
    pid_t pid;

    if (proc_id == NULL || path == NULL || argv == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    pid = fork();

    if (pid < 0) {
        /* fork失败 */
        return OSAL_ERR_NO_FREE_IDS;
    }

    if (pid == 0) {
        /* 子进程 */
        if (envp != NULL) {
            execve(path, argv, envp);
        } else {
            execv(path, argv);
        }

        /* 如果exec失败，退出子进程 */
        exit(1);
    }

    /* 父进程 */
    *proc_id = (osal_id_t)pid;
    return OSAL_SUCCESS;
}

int32_t OSAL_ProcessWait(osal_id_t proc_id, int32_t *status, int32_t timeout_ms)
{
    pid_t pid;
    int wait_status;
    int options;
    pid_t result;

    if (proc_id == 0) {
        return OSAL_ERR_INVALID_ID;
    }

    pid = (pid_t)proc_id;
    options = (timeout_ms == 0) ? WNOHANG : 0;

    result = waitpid(pid, &wait_status, options);

    if (result > 0) {
        if (status != NULL) {
            *status = WIFEXITED(wait_status) ? WEXITSTATUS(wait_status) : -1;
        }
        return OSAL_SUCCESS;
    }

    if (result < 0) {
        if (errno == ECHILD) {
            return OSAL_ERR_INVALID_ID;
        }
        return OSAL_ERR_GENERIC;
    }

    /* result == 0: 进程还在运行（仅WNOHANG时） */
    return OSAL_ERR_TIMEOUT;
}

int32_t OSAL_ProcessKill(osal_id_t proc_id, int32_t signal)
{
    pid_t pid;

    if (proc_id == 0) {
        return OSAL_ERR_INVALID_ID;
    }

    pid = (pid_t)proc_id;

    if (kill(pid, signal) == 0) {
        return OSAL_SUCCESS;
    }

    if (errno == ESRCH) {
        /* 进程不存在 */
        return OSAL_ERR_INVALID_ID;
    }

    return OSAL_ERR_GENERIC;
}

bool OSAL_ProcessExists(osal_id_t proc_id)
{
    pid_t pid;

    if (proc_id == 0) {
        return false;
    }

    pid = (pid_t)proc_id;

    /* 发送信号0检查进程是否存在 */
    if (kill(pid, 0) == 0) {
        return true;
    }

    return (errno != ESRCH);
}

osal_id_t OSAL_ProcessGetId(void)
{
    return (osal_id_t)getpid();
}

osal_id_t OSAL_ProcessGetParentId(void)
{
    return (osal_id_t)getppid();
}

int32_t OSAL_Fork(osal_id_t *child_pid)
{
    pid_t pid;

    if (child_pid == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    pid = fork();

    if (pid < 0) {
        /* fork失败 */
        return OSAL_ERR_NO_FREE_IDS;
    }

    /* 父进程中pid > 0，子进程中pid == 0 */
    *child_pid = (osal_id_t)pid;
    return OSAL_SUCCESS;
}

int32_t OSAL_Waitpid(osal_id_t pid, int32_t *status, int32_t options)
{
    int wait_status;
    int posix_options;
    pid_t result;

    posix_options = 0;

    /* 转换选项 */
    if (options & OSAL_WNOHANG) {
        posix_options |= WNOHANG;
    }

    result = waitpid((pid_t)pid, &wait_status, posix_options);

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
    if (errno == ECHILD) {
        return OSAL_ERR_INVALID_ID;
    }

    return OSAL_ERR_GENERIC;
}
