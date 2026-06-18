/************************************************************************
 * HAL Linux common locking helpers
 *
 * This file contains internal helpers shared by Linux HAL backends.
 ************************************************************************/

#ifndef HAL_LINUX_LOCK_H
#define HAL_LINUX_LOCK_H

#include "osal.h"

static inline int32_t _hal_linux_lock_device(osal_flock_t *flock,
											 osal_mutex_t *mutex,
											 uint32_t timeout_ms,
											 const char *log_module)
{
	int32_t ret;

	if (flock == NULL || mutex == NULL) {
		return OSAL_ERR_INVALID_POINTER;
	}

	ret = osal_flock_timed_lock(flock, OSAL_FLOCK_EXCLUSIVE, timeout_ms);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR(log_module, "Failed to acquire file lock (timeout or error)");
		return ret;
	}

	ret = osal_mutex_lock(mutex);
	if (ret != OSAL_SUCCESS) {
		LOG_ERROR(log_module, "Failed to acquire mutex");
		osal_flock_unlock(flock);
		return ret;
	}

	return OSAL_SUCCESS;
}

static inline void _hal_linux_unlock_device(osal_flock_t *flock,
											osal_mutex_t *mutex)
{
	if (mutex != NULL) {
		osal_mutex_unlock(mutex);
	}

	if (flock != NULL) {
		osal_flock_unlock(flock);
	}
}

#endif /* HAL_LINUX_LOCK_H */
