/************************************************************************
 * 时间服务API
 ************************************************************************/

#ifndef OSAPI_CLOCK_H
#define OSAPI_CLOCK_H

#include "osal_types.h"

/* 时间转换常量 */
#define OSAL_MS_PER_SEC         1000ULL     /* 每秒毫秒数 */
#define OSAL_NS_PER_MS          1000000ULL  /* 每毫秒纳秒数 */

/*
 * 时间结构
 */
typedef struct
{
    uint32_t seconds;      /* 秒 */
    uint32_t microsecs;    /* 微秒 */
} OS_time_t;

/**
 * @brief 获取本地时间
 *
 * @param[out] time_struct 时间结构
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER time_struct为NULL
 */
int32_t OSAL_GetLocalTime(OS_time_t *time_struct);

/**
 * @brief 设置本地时间
 *
 * @param[in] time_struct 时间结构
 *
 * @return OSAL_SUCCESS 成功
 * @return OSAL_ERR_INVALID_POINTER time_struct为NULL
 */
int32_t OSAL_SetLocalTime(const OS_time_t *time_struct);

/**
 * @brief 获取系统滴答计数
 *
 * @return 系统滴答数(毫秒)
 */
uint32_t OSAL_GetTickCount(void);

/**
 * @brief 毫秒延时
 *
 * @param[in] milliseconds 延时时间(毫秒)
 *
 * @return OSAL_SUCCESS 成功
 */
int32_t OSAL_Milli2Ticks(uint32_t milliseconds, uint32_t *ticks);

#endif /* OSAPI_CLOCK_H */
