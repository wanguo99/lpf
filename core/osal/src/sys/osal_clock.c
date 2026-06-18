/************************************************************************
 * OSAL POSIX实现 - 时间服务
 ************************************************************************/

#include "osal.h"
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

/* 时间API */
int32_t osal_get_local_time(OS_time_t *time_struct)
{
	struct timeval tv;

	if (NULL == time_struct)
		return OSAL_ERR_INVALID_POINTER;

	gettimeofday(&tv, NULL);
	time_struct->seconds = tv.tv_sec;
	time_struct->microsecs = tv.tv_usec;

	return OSAL_SUCCESS;
}

uint32_t osal_get_tick_count(void)
{
	struct timespec ts;
	uint64_t sec_ms;
	uint64_t nsec_ms;
	uint64_t total_ms;
	uint32_t result;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	/* 防止溢出：使用64位计算 */
	sec_ms = ts.tv_sec;
	sec_ms *= OSAL_MS_PER_SEC;
	nsec_ms = ts.tv_nsec / OSAL_NS_PER_MS;
	total_ms = sec_ms + nsec_ms;

	/* 返回低32位（约49.7天后会回绕） */
	result = total_ms;
	return result;
}
