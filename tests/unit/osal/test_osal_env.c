/**
 * @file test_osal_env.c
 * @brief OSAL环境变量操作单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"
#include <string.h>

/*===========================================================================
 * 测试用例
 *===========================================================================*/

TEST_CASE(test_osal_getenv_existing)
{
    /* Get an existing environment variable */
    char *value = OSAL_getenv("PATH");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_TRUE(strlen(value) > 0);
}

TEST_CASE(test_osal_getenv_nonexistent)
{
    /* Get a non-existent environment variable */
    char *value = OSAL_getenv("OSAL_NONEXISTENT_VAR_12345");
    TEST_ASSERT_NULL(value);
}

TEST_CASE(test_osal_setenv_new_variable)
{
    /* Set a new environment variable */
    int32_t ret = OSAL_setenv("OSAL_TEST_VAR", "test_value", 1);
    TEST_ASSERT_EQUAL(0, ret);

    /* Verify it was set */
    char *value = OSAL_getenv("OSAL_TEST_VAR");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_STRING_EQUAL("test_value", value);

    /* Clean up */
    OSAL_unsetenv("OSAL_TEST_VAR");
}

TEST_CASE(test_osal_setenv_overwrite)
{
    /* Set initial value */
    OSAL_setenv("OSAL_TEST_VAR2", "initial", 1);

    /* Overwrite with new value */
    int32_t ret = OSAL_setenv("OSAL_TEST_VAR2", "updated", 1);
    TEST_ASSERT_EQUAL(0, ret);

    /* Verify it was updated */
    char *value = OSAL_getenv("OSAL_TEST_VAR2");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_STRING_EQUAL("updated", value);

    /* Clean up */
    OSAL_unsetenv("OSAL_TEST_VAR2");
}

TEST_CASE(test_osal_setenv_no_overwrite)
{
    /* Set initial value */
    OSAL_setenv("OSAL_TEST_VAR3", "initial", 1);

    /* Try to set without overwrite flag */
    int32_t ret = OSAL_setenv("OSAL_TEST_VAR3", "should_not_change", 0);
    TEST_ASSERT_EQUAL(0, ret);

    /* Verify original value is preserved */
    char *value = OSAL_getenv("OSAL_TEST_VAR3");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_STRING_EQUAL("initial", value);

    /* Clean up */
    OSAL_unsetenv("OSAL_TEST_VAR3");
}

TEST_CASE(test_osal_unsetenv_existing)
{
    /* Set a variable */
    OSAL_setenv("OSAL_TEST_VAR4", "temp_value", 1);

    /* Unset it */
    int32_t ret = OSAL_unsetenv("OSAL_TEST_VAR4");
    TEST_ASSERT_EQUAL(0, ret);

    /* Verify it's gone */
    char *value = OSAL_getenv("OSAL_TEST_VAR4");
    TEST_ASSERT_NULL(value);
}

TEST_CASE(test_osal_unsetenv_nonexistent)
{
    /* Unset a non-existent variable (should succeed) */
    int32_t ret = OSAL_unsetenv("OSAL_NONEXISTENT_VAR_67890");
    TEST_ASSERT_EQUAL(0, ret);
}

TEST_CASE(test_osal_env_empty_value)
{
    /* Set variable with empty value */
    int32_t ret = OSAL_setenv("OSAL_TEST_VAR5", "", 1);
    TEST_ASSERT_EQUAL(0, ret);

    /* Verify it exists but is empty */
    char *value = OSAL_getenv("OSAL_TEST_VAR5");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_STRING_EQUAL("", value);

    /* Clean up */
    OSAL_unsetenv("OSAL_TEST_VAR5");
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

TEST_SUITE_BEGIN(test_osal_env, "osal_env", "OSAL")
    TEST_CASE_REF(test_osal_getenv_existing)
    TEST_CASE_REF(test_osal_getenv_nonexistent)
    TEST_CASE_REF(test_osal_setenv_new_variable)
    TEST_CASE_REF(test_osal_setenv_overwrite)
    TEST_CASE_REF(test_osal_setenv_no_overwrite)
    TEST_CASE_REF(test_osal_unsetenv_existing)
    TEST_CASE_REF(test_osal_unsetenv_nonexistent)
    TEST_CASE_REF(test_osal_env_empty_value)
TEST_SUITE_END(test_osal_env, "test_osal_env", "OSAL")
