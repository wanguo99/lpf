/************************************************************************
 * OSAL POSIX 后端 - 互斥锁
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <errno.h>
#include <time.h>

int32_t osal_mutex_init(osal_mutex_t *mutex, const osal_mutex_attr_t *attr)
{
	if (mutex == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutex_init(mutex, attr);
}

int32_t osal_mutex_destroy(osal_mutex_t *mutex)
{
	if (mutex == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutex_destroy(mutex);
}

int32_t osal_mutex_lock(osal_mutex_t *mutex)
{
	if (mutex == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutex_lock(mutex);
}

int32_t osal_mutex_try_lock(osal_mutex_t *mutex)
{
	if (mutex == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutex_trylock(mutex);
}

int32_t osal_mutex_timed_lock(osal_mutex_t *mutex, uint32_t timeout_ms)
{
	struct timespec ts;

	if (mutex == NULL) {
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

	return pthread_mutex_timedlock(mutex, &ts);
}

int32_t osal_mutex_unlock(osal_mutex_t *mutex)
{
	if (mutex == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutex_unlock(mutex);
}

/*===========================================================================
 * 互斥锁属性管理
 *===========================================================================*/

int32_t osal_mutex_attr_init(osal_mutex_attr_t *attr)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutexattr_init(attr);
}

int32_t osal_mutex_attr_destroy(osal_mutex_attr_t *attr)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutexattr_destroy(attr);
}

int32_t osal_mutex_attr_set_type(osal_mutex_attr_t *attr, int32_t type)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutexattr_settype(attr, type);
}

int32_t osal_mutex_attr_set_protocol(osal_mutex_attr_t *attr, int32_t protocol)
{
	if (attr == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutexattr_setprotocol(attr, protocol);
}

int32_t osal_mutex_attr_get_type(const osal_mutex_attr_t *attr, int32_t *type)
{
	if (attr == NULL || type == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_mutexattr_gettype(attr, type);
}
