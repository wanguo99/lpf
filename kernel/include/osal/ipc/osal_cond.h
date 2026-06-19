// SPDX-License-Identifier: GPL-2.0

#ifndef OSAL_COND_H
#define OSAL_COND_H

#include <linux/atomic.h>
#include <linux/wait.h>

#include "ipc/osal_mutex.h"
#include "osal_types.h"

typedef struct {
	wait_queue_head_t waitq;
	atomic_t generation;
} osal_cond_t;

typedef struct {
	uint32_t reserved;
} osal_cond_attr_t;

int32_t osal_cond_init(osal_cond_t *cond, const osal_cond_attr_t *attr);
int32_t osal_cond_destroy(osal_cond_t *cond);
int32_t osal_cond_wait(osal_cond_t *cond, osal_mutex_t *mutex);
int32_t osal_cond_timed_wait(osal_cond_t *cond, osal_mutex_t *mutex,
			     uint32_t timeout_ms);
int32_t osal_cond_signal(osal_cond_t *cond);
int32_t osal_cond_broadcast(osal_cond_t *cond);

#endif /* OSAL_COND_H */
