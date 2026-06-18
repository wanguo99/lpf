/************************************************************************
 * OSAL POSIX 后端 - 条件变量
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <errno.h>
#include <time.h>

int32_t osal_cond_init(osal_cond_t *cond, const osal_cond_attr_t *attr)
{
	if (cond == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_cond_init(cond, attr);
}

int32_t osal_cond_destroy(osal_cond_t *cond)
{
	if (cond == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_cond_destroy(cond);
}

int32_t osal_cond_wait(osal_cond_t *cond, osal_mutex_t *mutex)
{
	if (cond == NULL || mutex == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_cond_wait(cond, mutex);
}

int32_t osal_cond_timed_wait(osal_cond_t *cond, osal_mutex_t *mutex,
							 uint32_t timeout_ms)
{
	struct timespec ts;

	if (cond == NULL || mutex == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
		return -1;
	}

	ts.tv_sec += timeout_ms / 1000;
	ts.tv_nsec += (timeout_ms % 1000) * 1000000;
	if (ts.tv_nsec >= 1000000000) {
		ts.tv_sec += 1;
		ts.tv_nsec -= 1000000000;
	}

	return pthread_cond_timedwait(cond, mutex, &ts);
}

int32_t osal_cond_signal(osal_cond_t *cond)
{
	if (cond == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_cond_signal(cond);
}

int32_t osal_cond_broadcast(osal_cond_t *cond)
{
	if (cond == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_cond_broadcast(cond);
}
