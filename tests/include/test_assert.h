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

/* Core assertion - fails test but continues */
#define TEST_EXPECT(condition) \
    do { \
        if (!(condition)) { \
            OSAL_Printf("[  FAILED  ] %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            g_test_failed = true; \
        } \
    } while(0)

/* Fatal assertion - fails test and returns immediately */
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            OSAL_Printf("[  FAILED  ] %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Comparison assertions */
#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Expected %ld, got %ld\n", \
                   __FILE__, __LINE__, (intptr_t)(expected), (intptr_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_EXPECT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Expected %ld, got %ld\n", \
                   __FILE__, __LINE__, (intptr_t)(expected), (intptr_t)(actual)); \
            g_test_failed = true; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Values should not be equal: %ld\n", \
                   __FILE__, __LINE__, (intptr_t)(expected)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Pointer assertions */
#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Expected NULL pointer\n", \
                   __FILE__, __LINE__); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Expected non-NULL pointer\n", \
                   __FILE__, __LINE__); \
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
            OSAL_Printf("[  FAILED  ] %s:%d: Expected > %d, got %d\n", \
                   __FILE__, __LINE__, (int32_t)(threshold), (int32_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    do { \
        if ((actual) >= (threshold)) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Expected < %d, got %d\n", \
                   __FILE__, __LINE__, (int32_t)(threshold), (int32_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_GREATER_OR_EQUAL(threshold, actual) \
    do { \
        if ((actual) < (threshold)) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Expected >= %d, got %d\n", \
                   __FILE__, __LINE__, (int32_t)(threshold), (int32_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_LESS_OR_EQUAL(threshold, actual) \
    do { \
        if ((actual) > (threshold)) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Expected <= %d, got %d\n", \
                   __FILE__, __LINE__, (int32_t)(threshold), (int32_t)(actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* String assertions */
#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    do { \
        if (OSAL_Strcmp((expected), (actual)) != 0) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Expected \"%s\", got \"%s\"\n", \
                   __FILE__, __LINE__, (expected), (actual)); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Memory assertions */
#define TEST_ASSERT_MEMORY_EQUAL(expected, actual, size) \
    do { \
        if (OSAL_Memcmp((expected), (actual), (size)) != 0) { \
            OSAL_Printf("[  FAILED  ] %s:%d: Memory contents differ\n", \
                   __FILE__, __LINE__); \
            g_test_failed = true; \
            return; \
        } \
    } while(0)

/* Test control */
#define TEST_SKIP() \
    do { \
        OSAL_Printf("[  SKIPPED ] %s\n", g_current_test); \
        g_current_test = NULL; \
        return; \
    } while(0)

#define TEST_SKIP_IF(condition, message) \
    do { \
        if (condition) { \
            OSAL_Printf("[  SKIPPED ] %s: %s\n", g_current_test, message); \
            g_current_test = NULL; \
            return; \
        } \
    } while(0)

/* Informational messages */
#define TEST_MESSAGE(msg) \
    OSAL_Printf("[   INFO   ] %s\n", msg)

#define TEST_WARNING(msg) \
    OSAL_Printf("[ WARNING  ] %s\n", msg)

#endif /* TEST_ASSERT_H */
