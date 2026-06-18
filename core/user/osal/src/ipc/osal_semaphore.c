/************************************************************************
 * OSAL POSIX 后端 - 信号量
 ************************************************************************/

#include "osal.h"
#include <semaphore.h>
#include <errno.h>
#include <time.h>

int32_t osal_sem_init(osal_sem_t *sem, int32_t pshared, uint32_t value)
{
	if (sem == NULL) {
		errno = EINVAL;
		return -1;
	}

	return sem_init(sem, pshared, value);
}

int32_t osal_sem_destroy(osal_sem_t *sem)
{
	if (sem == NULL) {
		errno = EINVAL;
		return -1;
	}

	return sem_destroy(sem);
}

int32_t osal_sem_wait(osal_sem_t *sem)
{
	if (sem == NULL) {
		errno = EINVAL;
		return -1;
	}

	return sem_wait(sem);
}

int32_t osal_sem_try_wait(osal_sem_t *sem)
{
	if (sem == NULL) {
		errno = EINVAL;
		return -1;
	}

	return sem_trywait(sem);
}

int32_t osal_sem_timed_wait(osal_sem_t *sem, uint32_t timeout_ms)
{
	struct timespec ts;

	if (sem == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* 非阻塞模式 */
	if (timeout_ms == 0) {
		return sem_trywait(sem);
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

	return sem_timedwait(sem, &ts);
}

int32_t osal_sem_post(osal_sem_t *sem)
{
	if (sem == NULL) {
		errno = EINVAL;
		return -1;
	}

	return sem_post(sem);
}

int32_t osal_sem_get_value(osal_sem_t *sem, int32_t *value)
{
	int val;

	if (sem == NULL || value == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (sem_getvalue(sem, &val) != 0) {
		return -1;
	}

	*value = val;
	return 0;
}
