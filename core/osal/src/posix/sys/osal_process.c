/************************************************************************
 * OSAL - 进程管理接口实现（POSIX）
 ************************************************************************/

#include "sys/osal_process.h"
#include <osal.h>
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
    if (proc_id == NULL || path == NULL || argv == NULL) {
        return OSAL_ERR_INVALID_POINTER;
    }

    pid_t pid = fork();

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
    if (proc_id == 0) {
        return OSAL_ERR_INVALID_ID;
    }

    pid_t pid = (pid_t)proc_id;
    int wait_status;
    int options = (timeout_ms == 0) ? WNOHANG : 0;

    pid_t result = waitpid(pid, &wait_status, options);

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
    if (proc_id == 0) {
        return OSAL_ERR_INVALID_ID;
    }

    pid_t pid = (pid_t)proc_id;

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
    if (proc_id == 0) {
        return false;
    }

    pid_t pid = (pid_t)proc_id;

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
