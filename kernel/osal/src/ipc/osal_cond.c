// SPDX-License-Identifier: GPL-2.0

#include "osal/osal.h"

#include <linux/jiffies.h>
#include <linux/wait.h>

int32_t osal_cond_init(osal_cond_t *cond, const osal_cond_attr_t *attr)
{
	(void)attr;

	if (!cond)
		return OSAL_ERR_INVALID_POINTER;

	init_waitqueue_head(&cond->waitq);
	atomic_set(&cond->generation, 0);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_cond_init);

int32_t osal_cond_destroy(osal_cond_t *cond)
{
	if (!cond)
		return OSAL_ERR_INVALID_POINTER;

	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_cond_destroy);

static int32_t osal_cond_wait_common(osal_cond_t *cond, osal_mutex_t *mutex,
				     uint32_t timeout_ms, bool timed)
{
	long ret;
	int generation;

	if (!cond || !mutex)
		return OSAL_ERR_INVALID_POINTER;

	generation = atomic_read(&cond->generation);
	osal_mutex_unlock(mutex);

	if (timed) {
		ret = wait_event_interruptible_timeout(
			cond->waitq,
			atomic_read(&cond->generation) != generation,
			msecs_to_jiffies(timeout_ms));
	} else {
		ret = wait_event_interruptible(
			cond->waitq,
			atomic_read(&cond->generation) != generation);
	}

	osal_mutex_lock(mutex);

	if (ret == 0 && timed)
		return OSAL_ERR_TIMEOUT;
	if (ret < 0)
		return OSAL_ERR_INTERRUPTED;

	return OSAL_SUCCESS;
}

int32_t osal_cond_wait(osal_cond_t *cond, osal_mutex_t *mutex)
{
	return osal_cond_wait_common(cond, mutex, 0, false);
}
EXPORT_SYMBOL_GPL(osal_cond_wait);

int32_t osal_cond_timed_wait(osal_cond_t *cond, osal_mutex_t *mutex,
			     uint32_t timeout_ms)
{
	if (timeout_ms == 0)
		return osal_cond_wait_common(cond, mutex, 0, true);

	return osal_cond_wait_common(cond, mutex, timeout_ms, true);
}
EXPORT_SYMBOL_GPL(osal_cond_timed_wait);

int32_t osal_cond_signal(osal_cond_t *cond)
{
	if (!cond)
		return OSAL_ERR_INVALID_POINTER;

	atomic_inc(&cond->generation);
	wake_up_interruptible(&cond->waitq);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_cond_signal);

int32_t osal_cond_broadcast(osal_cond_t *cond)
{
	if (!cond)
		return OSAL_ERR_INVALID_POINTER;

	atomic_inc(&cond->generation);
	wake_up_interruptible_all(&cond->waitq);
	return OSAL_SUCCESS;
}
EXPORT_SYMBOL_GPL(osal_cond_broadcast);
