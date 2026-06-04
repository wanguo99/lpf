/**
 * @file test_osal_version.c
 * @brief OSAL版本信息单元测试
 */

#include "test_framework.h"
#include "osal/osal.h"

/*===========================================================================
 * 测试用例
 *===========================================================================*/

TEST_CASE(test_osal_get_version_string)
{
    const char *version = OSAL_GetVersionString();

    TEST_ASSERT_NOT_NULL(version);
    TEST_ASSERT_TRUE(OSAL_Strlen(version) > 0);

    /* Version string should contain "OSAL" */
    TEST_ASSERT_TRUE(OSAL_Strstr(version, "OSAL") != NULL);
}

TEST_CASE(test_osal_version_format)
{
    const char *version = OSAL_GetVersionString();

    /* Version should contain version number pattern */
    bool has_v = (OSAL_Strstr(version, "v") != NULL || OSAL_Strstr(version, "V") != NULL);
    bool has_dot = false;

    /* Check for dot manually */
    for (const char *p = version; *p != '\0'; p++) {
        if (*p == '.') {
            has_dot = true;
            break;
        }
    }

    TEST_ASSERT_TRUE(has_v || has_dot);
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

TEST_MODULE_BEGIN(test_osal_version, "OSAL")
    TEST_CASE_REGISTER(test_osal_get_version_string, "Get version string")
    TEST_CASE_REGISTER(test_osal_version_format, "Version format check")
    TEST_CASE_REGISTER(test_osal_version_consistency, "Version consistency")
TEST_MODULE_END(test_osal_version, "OSAL")
