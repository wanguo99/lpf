/************************************************************************
 * HAL Linux common device helpers
 *
 * This file contains internal helpers shared by Linux HAL backends.
 ************************************************************************/

#ifndef HAL_LINUX_LOCK_H
#define HAL_LINUX_LOCK_H

#include "osal.h"

typedef struct {
    osal_flock_t *flock;
    osal_mutex_t mutex;
    bool initialized;
} hal_linux_lock_t;

static inline void _hal_linux_close_fd(int32_t *fd)
{
    if (fd == NULL || *fd < 0)
        return;

    osal_close(*fd);
    *fd = -1;
}

static inline int32_t _hal_linux_lock_init(hal_linux_lock_t *lock,
                                           const char *lock_file,
                                           const char *log_module)
{
    int32_t ret;

    if (lock == NULL || lock_file == NULL || log_module == NULL)
        return OSAL_ERR_INVALID_POINTER;

    osal_memset(lock, 0, OSAL_sizeof(*lock));

    ret = osal_flock_create(lock_file, &lock->flock);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR(log_module, "Failed to create file lock: %s", lock_file);
        return ret;
    }

    ret = osal_mutex_init(&lock->mutex, NULL);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR(log_module, "Failed to create mutex");
        osal_flock_destroy(lock->flock);
        lock->flock = NULL;
        return ret;
    }

    lock->initialized = true;
    return OSAL_SUCCESS;
}

static inline void _hal_linux_lock_deinit(hal_linux_lock_t *lock)
{
    if (lock == NULL)
        return;

    if (lock->initialized)
        osal_mutex_destroy(&lock->mutex);

    if (lock->flock != NULL)
        osal_flock_destroy(lock->flock);

    osal_memset(lock, 0, OSAL_sizeof(*lock));
}

static inline int32_t _hal_linux_lock_acquire(hal_linux_lock_t *lock,
                                              uint32_t timeout_ms,
                                              const char *log_module)
{
    int32_t ret;

    if (lock == NULL || log_module == NULL)
        return OSAL_ERR_INVALID_POINTER;

    if (!lock->initialized)
        return OSAL_ERR_INVALID_ID;

    ret = osal_flock_timed_lock(lock->flock, OSAL_FLOCK_EXCLUSIVE, timeout_ms);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR(log_module, "Failed to acquire file lock (timeout or error)");
        return ret;
    }

    ret = osal_mutex_lock(&lock->mutex);
    if (ret != OSAL_SUCCESS) {
        LOG_ERROR(log_module, "Failed to acquire mutex");
        osal_flock_unlock(lock->flock);
        return ret;
    }

    return OSAL_SUCCESS;
}

static inline void _hal_linux_lock_release(hal_linux_lock_t *lock)
{
    if (lock == NULL || !lock->initialized)
        return;

    osal_mutex_unlock(&lock->mutex);
    osal_flock_unlock(lock->flock);
}

#endif /* HAL_LINUX_LOCK_H */
