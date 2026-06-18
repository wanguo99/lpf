/************************************************************************
 * Kernel OSAL public interface
 *
 * This header exposes the small kernel-mode OSAL surface used by kernel
 * modules. It intentionally does not mirror the userspace OSAL API.
 ************************************************************************/

#ifndef OSAL_KERNEL_H
#define OSAL_KERNEL_H

#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>

typedef struct mutex osal_kmutex_t;

static inline void *osal_kmalloc(size_t size)
{
	return kmalloc(size, GFP_KERNEL);
}

static inline void *osal_kzalloc(size_t size)
{
	return kzalloc(size, GFP_KERNEL);
}

static inline void osal_kfree(const void *ptr)
{
	kfree(ptr);
}

static inline void osal_kmutex_init(osal_kmutex_t *lock)
{
	mutex_init(lock);
}

static inline void osal_kmutex_lock(osal_kmutex_t *lock)
{
	mutex_lock(lock);
}

static inline void osal_kmutex_unlock(osal_kmutex_t *lock)
{
	mutex_unlock(lock);
}

const char *osal_kernel_version(void);

#endif /* OSAL_KERNEL_H */
