/************************************************************************
 * OSAL POSIX 后端 - 读写锁
 ************************************************************************/

#include "osal.h"
#include <pthread.h>
#include <errno.h>

int32_t osal_rwlock_init(osal_rwlock_t *rwlock, const osal_rwlock_attr_t *attr)
{
	if (rwlock == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_rwlock_init(rwlock, attr);
}

int32_t osal_rwlock_destroy(osal_rwlock_t *rwlock)
{
	if (rwlock == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_rwlock_destroy(rwlock);
}

int32_t osal_rwlock_read_lock(osal_rwlock_t *rwlock)
{
	if (rwlock == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_rwlock_rdlock(rwlock);
}

int32_t osal_rwlock_write_lock(osal_rwlock_t *rwlock)
{
	if (rwlock == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_rwlock_wrlock(rwlock);
}

int32_t osal_rwlock_try_read_lock(osal_rwlock_t *rwlock)
{
	if (rwlock == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_rwlock_tryrdlock(rwlock);
}

int32_t osal_rwlock_try_write_lock(osal_rwlock_t *rwlock)
{
	if (rwlock == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_rwlock_trywrlock(rwlock);
}

int32_t osal_rwlock_unlock(osal_rwlock_t *rwlock)
{
	if (rwlock == NULL) {
		errno = EINVAL;
		return -1;
	}

	return pthread_rwlock_unlock(rwlock);
}
