/************************************************************************
 * Kernel OSAL compatibility entry
 *
 * This header is the kernel-side OSAL aggregation point. It keeps the public
 * API names aligned with userspace OSAL where the kernel can provide matching
 * semantics.
 ************************************************************************/

#ifndef OSAL_H
#define OSAL_H

/*
 * OSAL version information
 */
#define OSAL_LITE_VERSION_MAJOR 0x1
#define OSAL_LITE_VERSION_MINOR 0x0
#define OSAL_LITE_VERSION_PATCH 0x0

#include <linux/kernel.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/byteorder/generic.h>

#include "osal_platform.h"
#include "osal_types.h"
#include "ipc/osal_atomic.h"
#include "ipc/osal_cond.h"
#include "ipc/osal_mutex.h"
#include "ipc/osal_semaphore.h"
#include "lib/osal_errno.h"
#include "lib/osal_heap.h"
#include "lib/osal_string.h"
#include "sys/osal_time.h"
#include "sys/osal_thread.h"
#include "sys/osal_usercopy.h"
#include "util/osal_log.h"
#include "util/osal_crc.h"
#include "util/osal_version.h"

typedef struct {
	uint32_t seconds;
	uint32_t microsecs;
} OS_time_t;

static inline int32_t osal_get_local_time(OS_time_t *time_struct)
{
	struct timespec64 ts;

	if (!time_struct)
		return OSAL_ERR_INVALID_POINTER;

	ktime_get_real_ts64(&ts);
	time_struct->seconds = (uint32_t)ts.tv_sec;
	time_struct->microsecs = (uint32_t)(ts.tv_nsec / NSEC_PER_USEC);

	return OSAL_SUCCESS;
}

static inline uint16_t osal_htons(uint16_t hostshort)
{
	return (uint16_t)htons(hostshort);
}

static inline uint32_t osal_htonl(uint32_t hostlong)
{
	return (uint32_t)htonl(hostlong);
}

static inline uint16_t osal_ntohs(uint16_t netshort)
{
	return (uint16_t)ntohs(netshort);
}

static inline uint32_t osal_ntohl(uint32_t netlong)
{
	return (uint32_t)ntohl(netlong);
}

#endif /* OSAL_H */
