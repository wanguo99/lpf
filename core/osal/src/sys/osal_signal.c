/************************************************************************
 * OSAL - POSIX信号薄封装实现
 ************************************************************************/

#include "osal.h"
#include <signal.h>
#include <errno.h>

int32_t osal_signal(int32_t signum, osal_sighandler_t handler)
{
	if (signal(signum, handler) == SIG_ERR) {
		return -1;
	}
	return 0;
}

int32_t osal_kill(osal_pid_t pid, int32_t sig)
{
	return kill(pid, sig);
}

int32_t osal_raise(int32_t sig)
{
	return raise(sig);
}

int32_t osal_sigprocmask(int32_t how, const sigset_t *set, sigset_t *oldset)
{
	return sigprocmask(how, set, oldset);
}

int32_t osal_sigemptyset(sigset_t *set)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}
	return sigemptyset(set);
}

int32_t osal_sigfillset(sigset_t *set)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}
	return sigfillset(set);
}

int32_t osal_sigaddset(sigset_t *set, int32_t signum)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}
	return sigaddset(set, signum);
}

int32_t osal_sigdelset(sigset_t *set, int32_t signum)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}
	return sigdelset(set, signum);
}

int32_t osal_sigismember(const sigset_t *set, int32_t signum)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}
	return sigismember(set, signum);
}

int32_t osal_sigwait(const sigset_t *set, int32_t *sig)
{
	if (set == NULL) {
		errno = EINVAL;
		return -1;
	}
	return sigwait(set, sig);
}

int32_t osal_sigaction(int32_t signum, const struct sigaction *act,
					   struct sigaction *oldact)
{
	return sigaction(signum, act, oldact);
}
