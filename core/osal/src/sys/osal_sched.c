/************************************************************************
 * OSAL POSIX 后端 - 实时调度
 ************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "osal.h"
#include <pthread.h>
#include <sched.h>
#include <errno.h>

int32_t osal_thread_set_sched_param(osal_thread_t thread, int32_t policy,
									int32_t priority)
{
	osal_sched_param_t param;
	param.sched_priority = priority;

	return pthread_setschedparam(thread, policy, &param);
}

int32_t osal_thread_get_sched_param(osal_thread_t thread, int32_t *policy,
									int32_t *priority)
{
	osal_sched_param_t param;
	int pol;
	int ret;

	ret = pthread_getschedparam(thread, &pol, &param);
	if (ret == 0) {
		if (policy != NULL) {
			*policy = pol;
		}
		if (priority != NULL) {
			*priority = param.sched_priority;
		}
	}

	return ret;
}

int32_t osal_thread_set_affinity(osal_thread_t thread, int32_t cpu_id)
{
#ifdef __linux__
	cpu_set_t cpuset;

	if (cpu_id < 0) {
		errno = EINVAL;
		return -1;
	}

	CPU_ZERO(&cpuset);
	CPU_SET(cpu_id, &cpuset);

	return pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#else
	(void)thread;
	(void)cpu_id;
	errno = ENOSYS;
	return -1;
#endif
}

int32_t osal_thread_get_affinity(osal_thread_t thread, int32_t *cpu_id)
{
#ifdef __linux__
	cpu_set_t cpuset;
	int ret;
	int i;

	if (cpu_id == NULL) {
		errno = EINVAL;
		return -1;
	}

	ret = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (ret == 0) {
		/* 返回第一个设置的 CPU */
		for (i = 0; i < CPU_SETSIZE; i++) {
			if (CPU_ISSET(i, &cpuset)) {
				*cpu_id = i;
				return 0;
			}
		}
	}

	return ret;
#else
	(void)thread;
	(void)cpu_id;
	errno = ENOSYS;
	return -1;
#endif
}

int32_t osal_sched_set_affinity(osal_pid_t pid, int32_t cpu_id)
{
#ifdef __linux__
	cpu_set_t cpuset;

	if (cpu_id < 0) {
		errno = EINVAL;
		return -1;
	}

	CPU_ZERO(&cpuset);
	CPU_SET(cpu_id, &cpuset);

	return sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset);
#else
	(void)pid;
	(void)cpu_id;
	errno = ENOSYS;
	return -1;
#endif
}

int32_t osal_sched_get_affinity(osal_pid_t pid, int32_t *cpu_id)
{
#ifdef __linux__
	cpu_set_t cpuset;
	int ret;
	int i;

	if (cpu_id == NULL) {
		errno = EINVAL;
		return -1;
	}

	ret = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);
	if (ret == 0) {
		/* 返回第一个设置的 CPU */
		for (i = 0; i < CPU_SETSIZE; i++) {
			if (CPU_ISSET(i, &cpuset)) {
				*cpu_id = i;
				return 0;
			}
		}
	}

	return ret;
#else
	(void)pid;
	(void)cpu_id;
	errno = ENOSYS;
	return -1;
#endif
}

int32_t osal_sched_set_scheduler(osal_pid_t pid, int32_t policy,
								 const osal_sched_param_t *param)
{
	if (param == NULL) {
		errno = EINVAL;
		return -1;
	}

	return sched_setscheduler(pid, policy, param);
}

int32_t osal_sched_get_scheduler(osal_pid_t pid)
{
	return sched_getscheduler(pid);
}

int32_t osal_sched_get_priority_min(int32_t policy)
{
	return sched_get_priority_min(policy);
}

int32_t osal_sched_get_priority_max(int32_t policy)
{
	return sched_get_priority_max(policy);
}

int32_t osal_sched_yield(void)
{
	return sched_yield();
}
