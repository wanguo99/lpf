/**
 * @file test_osal_errno.c
 * @brief OSAL错误码操作单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "osal.h"
#include <string.h>

/*===========================================================================
 * 测试用例
 *===========================================================================*/

TEST_CASE(test_osal_get_errno)
{
    /* Set errno to a known value */
    OSAL_SetErrno(OSAL_EINVAL);

    /* Get errno */
    int32_t err = OSAL_GetErrno();
    TEST_ASSERT_EQUAL(OSAL_EINVAL, err);
}

TEST_CASE(test_osal_set_errno)
{
    /* Set errno to different values */
    OSAL_SetErrno(OSAL_ENOENT);
    TEST_ASSERT_EQUAL(OSAL_ENOENT, OSAL_GetErrno());

    OSAL_SetErrno(OSAL_ENOMEM);
    TEST_ASSERT_EQUAL(OSAL_ENOMEM, OSAL_GetErrno());

    OSAL_SetErrno(0);
    TEST_ASSERT_EQUAL(0, OSAL_GetErrno());
}

TEST_CASE(test_osal_strerror)
{
    /* Test common error codes */
    const char *msg;

    msg = OSAL_StrError(OSAL_EINVAL);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_TRUE(strlen(msg) > 0);

    msg = OSAL_StrError(OSAL_ENOENT);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_TRUE(strlen(msg) > 0);

    msg = OSAL_StrError(OSAL_ENOMEM);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_TRUE(strlen(msg) > 0);
}

TEST_CASE(test_osal_get_status_name)
{
    /* Test OSAL status code names */
    const char *name;

    name = OSAL_GetStatusName(OSAL_SUCCESS);
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_STRING_EQUAL("OSAL_SUCCESS", name);

    name = OSAL_GetStatusName(OSAL_ERR_INVALID_POINTER);
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_STRING_EQUAL("OSAL_ERR_INVALID_POINTER", name);

    name = OSAL_GetStatusName(OSAL_ERR_INVALID_SIZE);
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_STRING_EQUAL("OSAL_ERR_INVALID_SIZE", name);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

TEST_SUITE_BEGIN(test_osal_errno, "osal_errno", "OSAL")
    TEST_CASE_REF(test_osal_get_errno)
    TEST_CASE_REF(test_osal_set_errno)
    TEST_CASE_REF(test_osal_strerror)
    TEST_CASE_REF(test_osal_get_status_name)
TEST_SUITE_END(test_osal_errno, "test_osal_errno", "OSAL")
