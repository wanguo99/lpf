/**
 * @file test_assert.h
 * @brief Test assertion macros
 *
 * Provides a comprehensive set of assertion macros for unit testing.
 * Supports both fatal assertions (TEST_ASSERT_*) that stop test execution
 * and non-fatal expectations (TEST_EXPECT_*) that continue execution.
 */

#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include "osal.h"

/* Internal state tracking */
extern bool g_test_failed;
extern const char *g_current_test;

/* Fail test immediately */
#define TEST_FAIL() \
    do { \
        OSAL_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__); \
        g_test_failed = true; \
        return; \
    } while(0)

/* Core assertion - fails test but continues */
#define TEST_EXPECT(condition) \
    do { \
        if (!(condition)) { \
            OSAL_printf("[  FAILED  ] %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            g_test_failed = true; \
        } \
    } while(0)

/* Fatal assertion - fails test and returns immediately */
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            OSAL_printf("[  FAILED  ] %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Comparison assertions */
#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            OSAL_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__); \
            OSAL_printf("            Expression: %s == %s\n", #expected, #actual); \
            OSAL_printf("            Expected: %ld\n", (long)(expected)); \
            OSAL_printf("            Actual:   %ld\n", (long)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_EXPECT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            OSAL_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__); \
            OSAL_printf("            Expression: %s == %s\n", #expected, #actual); \
            OSAL_printf("            Expected: %ld\n", (long)(expected)); \
            OSAL_printf("            Actual:   %ld\n", (long)(actual)); \
            g_test_failed = true; \
        } \
    } while(0)

/* Type-safe comparison assertions (new) */

/* Integer comparisons */
#define TEST_ASSERT_INT_EQUAL(expected, actual) \
    do { \
        int32_t _test_exp = (expected); \
        int32_t _test_act = (actual); \
        if (_test_exp != _test_act) { \
            OSAL_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__); \
            OSAL_printf("            Expression: %s == %s\n", #expected, #actual); \
            OSAL_printf("            Expected: %d (int32_t)\n", _test_exp); \
            OSAL_printf("            Actual:   %d (int32_t)\n", _test_act); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Unsigned integer comparisons */
#define TEST_ASSERT_UINT_EQUAL(expected, actual) \
    do { \
        uint32_t _test_exp = (expected); \
        uint32_t _test_act = (actual); \
        if (_test_exp != _test_act) { \
            OSAL_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__); \
            OSAL_printf("            Expression: %s == %s\n", #expected, #actual); \
            OSAL_printf("            Expected: %u (uint32_t)\n", _test_exp); \
            OSAL_printf("            Actual:   %u (uint32_t)\n", _test_act); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Pointer comparisons */
#define TEST_ASSERT_PTR_EQUAL(expected, actual) \
    do { \
        const void *_test_exp = (const void *)(expected); \
        const void *_test_act = (const void *)(actual); \
        if (_test_exp != _test_act) { \
            OSAL_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__); \
            OSAL_printf("            Expression: %s == %s\n", #expected, #actual); \
            OSAL_printf("            Expected: %p (pointer)\n", _test_exp); \
            OSAL_printf("            Actual:   %p (pointer)\n", _test_act); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            OSAL_printf("[  FAILED  ] %s:%d: Values should not be equal: %ld\n", \
                   __FILE__, __LINE__, (intptr_t)(expected)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Pointer assertions */
#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            OSAL_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__); \
            OSAL_printf("            Expression: %s == NULL\n", #ptr); \
            OSAL_printf("            Expected: NULL\n"); \
            OSAL_printf("            Actual:   %p (non-NULL)\n", (void*)(ptr)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            OSAL_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__); \
            OSAL_printf("            Expression: %s != NULL\n", #ptr); \
            OSAL_printf("            Expected: non-NULL pointer\n"); \
            OSAL_printf("            Actual:   NULL\n"); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Boolean assertions */
#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

/* Range assertions */
#define TEST_ASSERT_GREATER_THAN(threshold, actual) \
    do { \
        if ((actual) <= (threshold)) { \
            OSAL_printf("[  FAILED  ] %s:%d: Expected > %d, got %d\n", \
                   __FILE__, __LINE__, (int32_t)(threshold), (int32_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    do { \
        if ((actual) >= (threshold)) { \
            OSAL_printf("[  FAILED  ] %s:%d: Expected < %d, got %d\n", \
                   __FILE__, __LINE__, (int32_t)(threshold), (int32_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_GREATER_OR_EQUAL(threshold, actual) \
    do { \
        if ((actual) < (threshold)) { \
            OSAL_printf("[  FAILED  ] %s:%d: Expected >= %d, got %d\n", \
                   __FILE__, __LINE__, (int32_t)(threshold), (int32_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_LESS_OR_EQUAL(threshold, actual) \
    do { \
        if ((actual) > (threshold)) { \
            OSAL_printf("[  FAILED  ] %s:%d: Expected <= %d, got %d\n", \
                   __FILE__, __LINE__, (int32_t)(threshold), (int32_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* String assertions */
#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    do { \
        if (OSAL_strcmp((expected), (actual)) != 0) { \
            OSAL_printf("[  FAILED  ] %s:%d: Expected \"%s\", got \"%s\"\n", \
                   __FILE__, __LINE__, (expected), (actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Floating point assertions - using simple integer-based comparison */
#define TEST_ASSERT_FLOAT_EQUAL(expected, actual, tolerance) \
    do { \
        double _exp = (double)(expected); \
        double _act = (double)(actual); \
        double _tol = (double)(tolerance); \
        double _diff = (_exp > _act) ? (_exp - _act) : (_act - _exp); \
        if (_diff > _tol) { \
            OSAL_printf("[  FAILED  ] %s:%d: Expected %.6f, got %.6f (tolerance %.6f, diff %.6f)\n", \
                   __FILE__, __LINE__, _exp, _act, _tol, _diff); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_DOUBLE_EQUAL(expected, actual, tolerance) \
    TEST_ASSERT_FLOAT_EQUAL(expected, actual, tolerance)

/* Memory assertions */
#define TEST_ASSERT_MEMORY_EQUAL(expected, actual, size) \
    do { \
        if (OSAL_memcmp((expected), (actual), (size)) != 0) { \
            OSAL_printf("[  FAILED  ] %s:%d: Memory contents differ\n", \
                   __FILE__, __LINE__); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Informational messages */
#define TEST_MESSAGE(msg) \
    OSAL_printf("[   INFO   ] %s\n", msg)

#define TEST_WARNING(msg) \
    OSAL_printf("[ WARNING  ] %s\n", msg)

#endif /* TEST_ASSERT_H */
