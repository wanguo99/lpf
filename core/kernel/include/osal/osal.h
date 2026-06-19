/************************************************************************
 * Kernel OSAL compatibility entry
 *
 * This header exposes the small OSAL subset required by kernel-side
 * PCONFIG/PDM code. It is intentionally not a mirror of userspace OSAL.
 ************************************************************************/

#ifndef OSAL_H
#define OSAL_H

#include <linux/atomic.h>
#include <linux/byteorder/generic.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timekeeping.h>
#include <linux/types.h>

#include "osal_kernel.h"

typedef size_t osal_size_t;

typedef struct {
	uint32_t seconds;
	uint32_t microsecs;
} OS_time_t;

#define OSAL_SUCCESS 0

#define OSAL_EPERM EPERM
#define OSAL_ENOENT ENOENT
#define OSAL_EIO EIO
#define OSAL_EFAULT EFAULT
#define OSAL_EBUSY EBUSY
#define OSAL_EINVAL EINVAL
#define OSAL_ENOMEM ENOMEM
#define OSAL_ENOSYS ENOSYS
#define OSAL_ETIMEDOUT ETIMEDOUT
#define OSAL_EPROTO EPROTO
#define OSAL_EOPNOTSUPP EOPNOTSUPP

#define OSAL_ERR_INVALID_POINTER OSAL_EFAULT
#define OSAL_ERR_NO_MEMORY OSAL_ENOMEM
#define OSAL_ERR_INVALID_SIZE OSAL_EINVAL
#define OSAL_ERR_INVALID_ID OSAL_EINVAL
#define OSAL_ERR_INVALID_PARAM OSAL_EINVAL
#define OSAL_ERR_TIMEOUT OSAL_ETIMEDOUT
#define OSAL_ERR_NOT_IMPLEMENTED OSAL_ENOSYS
#define OSAL_ERR_BUSY OSAL_EBUSY
#define OSAL_ERR_NOT_SUPPORTED OSAL_EOPNOTSUPP
#define OSAL_ERR_GENERIC OSAL_EIO

#define OSAL_sizeof(x) ((osal_size_t)sizeof(x))
#define OSAL_ARRAY_SIZE(arr) (OSAL_sizeof(arr) / OSAL_sizeof((arr)[0]))
#define OS_PEND 0

#define osal_memset memset
#define osal_memcpy memcpy
#define osal_memmove memmove
#define osal_memcmp memcmp
#define osal_strcmp strcmp
#define osal_strncmp strncmp
#define osal_strlen strlen
#define osal_strncpy strscpy

#define LOG_INFO(module, fmt, ...) \
	pr_info("ESMW:%s: " fmt "\n", module, ##__VA_ARGS__)
#define LOG_ERROR(module, fmt, ...) \
	pr_err("ESMW:%s: " fmt "\n", module, ##__VA_ARGS__)

typedef struct mutex osal_mutex_t;

static inline void *osal_malloc(size_t size)
{
	return kmalloc(size, GFP_KERNEL);
}

static inline void *osal_zalloc(size_t size)
{
	return kzalloc(size, GFP_KERNEL);
}

static inline void osal_free(const void *ptr)
{
	kfree(ptr);
}

static inline int32_t osal_mutex_init(osal_mutex_t *mutex, const void *attr)
{
	(void)attr;

	if (!mutex)
		return OSAL_ERR_INVALID_POINTER;

	mutex_init(mutex);
	return OSAL_SUCCESS;
}

static inline int32_t osal_mutex_lock(osal_mutex_t *mutex)
{
	if (!mutex)
		return OSAL_ERR_INVALID_POINTER;

	mutex_lock(mutex);
	return OSAL_SUCCESS;
}

static inline int32_t osal_mutex_unlock(osal_mutex_t *mutex)
{
	if (!mutex)
		return OSAL_ERR_INVALID_POINTER;

	mutex_unlock(mutex);
	return OSAL_SUCCESS;
}

static inline int32_t osal_mutex_destroy(osal_mutex_t *mutex)
{
	if (!mutex)
		return OSAL_ERR_INVALID_POINTER;

	return OSAL_SUCCESS;
}

typedef struct {
	atomic_t value;
} osal_atomic_uint32_t;

static inline void osal_atomic_init(osal_atomic_uint32_t *atomic,
				    uint32_t value)
{
	atomic_set(&atomic->value, (int)value);
}

static inline uint32_t osal_atomic_load(const osal_atomic_uint32_t *atomic)
{
	return (uint32_t)atomic_read(&atomic->value);
}

static inline void osal_atomic_store(osal_atomic_uint32_t *atomic,
				     uint32_t value)
{
	atomic_set(&atomic->value, (int)value);
}

static inline uint32_t osal_atomic_fetch_add(osal_atomic_uint32_t *atomic,
					     uint32_t value)
{
	return (uint32_t)atomic_fetch_add((int)value, &atomic->value);
}

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

static inline uint64_t osal_get_monotonic_time(void)
{
	return (uint64_t)(ktime_get_ns() / NSEC_PER_USEC);
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

uint16_t osal_crc16_ccitt(const uint8_t *data, size_t len);
uint16_t osal_crc16_ccitt_update(uint16_t crc, const uint8_t *data,
				 size_t len);
const char *osal_get_status_name(int32_t status_code);

#endif /* OSAL_H */
