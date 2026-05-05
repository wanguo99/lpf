/**
 * @file test_osal_version.c
 * @brief OSAL版本信息单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"
#include <string.h>

/*===========================================================================
 * 测试用例
 *===========================================================================*/

TEST_CASE(test_osal_get_version_string)
{
    const char *version = OSAL_GetVersionString();

    TEST_ASSERT_NOT_NULL(version);
    TEST_ASSERT_TRUE(strlen(version) > 0);

    /* Version string should contain "OSAL" */
    TEST_ASSERT_TRUE(strstr(version, "OSAL") != NULL);
}

TEST_CASE(test_osal_version_format)
{
    const char *version = OSAL_GetVersionString();

    /* Version should contain version number pattern */
    TEST_ASSERT_TRUE(strstr(version, "v") != NULL ||
                     strstr(version, "V") != NULL ||
                     strchr(version, '.') != NULL);
}

TEST_CASE(test_osal_version_consistency)
{
    /* Multiple calls should return the same string */
    const char *v1 = OSAL_GetVersionString();
    const char *v2 = OSAL_GetVersionString();

    TEST_ASSERT_STRING_EQUAL(v1, v2);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

TEST_SUITE_BEGIN(test_osal_version, "osal_version", "OSAL")
    TEST_CASE_REF(test_osal_get_version_string)
    TEST_CASE_REF(test_osal_version_format)
    TEST_CASE_REF(test_osal_version_consistency)
TEST_SUITE_END(test_osal_version, "test_osal_version", "OSAL")
