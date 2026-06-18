/************************************************************************
 * 时间服务API
 ************************************************************************/

#ifndef OSAL_CLOCK_H
#define OSAL_CLOCK_H

/* 时间转换常量 */
#define OSAL_MS_PER_SEC 0x3E8ULL /* 每秒毫秒数 */
#define OSAL_NS_PER_MS 0xF4240ULL /* 每毫秒纳秒数 */

/*
 * 时间结构
 */
typedef struct {
	uint32_t seconds; /* 秒 */
	uint32_t microsecs; /* 微秒 */
} OS_time_t;

/**
 * @brief 获取本地时间
 *
 * @param[out] time_struct 时间结构
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER time_struct为NULL
 */
int32_t osal_get_local_time(OS_time_t *time_struct);

/**
 * @brief 获取系统滴答计数
 *
 * @return 系统滴答数(毫秒)
 */
uint32_t osal_get_tick_count(void);

#endif /* OSAL_CLOCK_H */
