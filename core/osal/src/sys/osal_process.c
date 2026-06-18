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

void osal_exit(int32_t status)
{
	exit(status);
}

osal_pid_t osal_getpid(void)
{
	return getpid();
}

osal_pid_t osal_getppid(void)
{
	return getppid();
}

void osal_abort(void)
{
	abort();
}

osal_pid_t osal_fork(void)
{
	return fork();
}

int32_t osal_pipe(int32_t pipefd[2])
{
	return pipe(pipefd);
}

int32_t osal_execvp(const char *file, char *const argv[])
{
	return execvp(file, argv);
}

int32_t osal_waitpid(osal_pid_t pid, int32_t *status, int32_t options)
{
	int wait_status;
	int posix_options = 0;
	osal_pid_t result;

	/* 转换选项 */
	if (options & OSAL_WNOHANG) {
		posix_options |= WNOHANG;
	}

	result = waitpid(pid, &wait_status, posix_options);

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

int32_t osal_setpgid(osal_pid_t pid, osal_pid_t pgid)
{
	return setpgid(pid, pgid);
}

osal_pid_t osal_getpgid(osal_pid_t pid)
{
	return getpgid(pid);
}

/*
 * 以下为过时的高层封装，将在后续版本中移除
 */
