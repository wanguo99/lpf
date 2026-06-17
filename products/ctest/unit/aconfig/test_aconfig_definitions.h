/**
 * @file test_aconfig_definitions.h
 * @brief AConfig 测试专用定义
 * @note 测试数据，不依赖具体产品（如 PMC）
 */

#ifndef TEST_ACONFIG_DEFINITIONS_H
#define TEST_ACONFIG_DEFINITIONS_H

#include "osal.h"

/* ========================================================================
 * 测试用遥控功能枚举（独立于产品）
 * ======================================================================== */

typedef enum {
    TEST_TC_POWER_ON = 0,
    TEST_TC_POWER_OFF,
    TEST_TC_RESET,
    TEST_TC_MAX
} test_tc_function_t;

/* ========================================================================
 * 测试用遥测功能枚举（独立于产品）
 * ======================================================================== */

typedef enum {
    TEST_TM_VOLTAGE = 0,
    TEST_TM_TEMPERATURE,
    TEST_TM_STATUS,
    TEST_TM_MAX
} test_tm_function_t;

#endif /* TEST_ACONFIG_DEFINITIONS_H */
