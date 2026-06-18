/************************************************************************
 * OSAL POSIX实现 - 时间延迟操作
 *
 * 修复：统一错误码返回，符合 OSAL 规范
 ************************************************************************/

#define _DEFAULT_SOURCE /* 启用usleep等函数 */
#include "osal.h"
#include <unistd.h>
#include <time.h>
#include <errno.h>

int32_t osal_msleep(uint32_t msec)
{
	if (usleep(msec * OSAL_USEC_PER_MSEC) == 0)
		return OSAL_SUCCESS;
	return OSAL_ERR_GENERIC;
}

int32_t osal_usleep(uint32_t usec)
{
	if (usleep(usec) == 0)
		return OSAL_SUCCESS;
	return OSAL_ERR_GENERIC;
}

int32_t osal_sleep(uint32_t sec)
{
	/* sleep() 返回剩余未睡眠的秒数，0 表示成功 */
	if (sleep(sec) == 0)
		return OSAL_SUCCESS;
	return OSAL_ERR_GENERIC;
}

int32_t osal_nanosleep(uint64_t nsec)
{
	struct timespec req;
	req.tv_sec = nsec / OSAL_NSEC_PER_SEC;
	req.tv_nsec = nsec % OSAL_NSEC_PER_SEC;

	if (nanosleep(&req, NULL) == 0)
		return OSAL_SUCCESS;
	return OSAL_ERR_GENERIC;
}

int64_t osal_get_monotonic_time(void)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return -1;

	return (int64_t)ts.tv_sec * OSAL_USEC_PER_SEC +
		   ts.tv_nsec / OSAL_NSEC_PER_USEC;
}

int64_t osal_get_boot_time(void)
{
#ifdef CLOCK_BOOTTIME
	struct timespec ts;
	if (clock_gettime(CLOCK_BOOTTIME, &ts) != 0)
		return -1;

	return (int64_t)ts.tv_sec * OSAL_USEC_PER_SEC +
		   ts.tv_nsec / OSAL_NSEC_PER_USEC;
#else
	/* 内核或 C 库未提供 CLOCK_BOOTTIME 时降级到 CLOCK_MONOTONIC */
	return osal_get_monotonic_time();
#endif
}

int64_t osal_get_highres_time(void)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return -1;

	return (int64_t)ts.tv_sec * OSAL_NSEC_PER_SEC + ts.tv_nsec;
}
