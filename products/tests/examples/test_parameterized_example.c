/**
 * @file test_parameterized_example.c
 * @brief 参数化测试示例
 *
 * 展示如何使用参数化测试功能
 */

#include "test_framework.h"
#include "osal.h"

/* 示例 1：简单的数学运算测试 */
typedef struct {
    int32_t a;
    int32_t b;
    int32_t expected_sum;
} add_test_data_t;

static add_test_data_t add_tests[] = {
    {1, 2, 3},
    {-1, 1, 0},
    {100, 200, 300},
    {0, 0, 0},
    {-5, -10, -15}
};

TEST_CASE_PARAMETERIZED(test_addition, add_test_data_t, add_tests)
{
    add_test_data_t *data = (add_test_data_t *)param;
    int32_t result = data->a + data->b;

    TEST_PARAM_INFO("Testing %d + %d = %d", data->a, data->b, data->expected_sum);
    TEST_ASSERT_INT_EQUAL(data->expected_sum, result);
}

/* 示例 2：字符串长度测试 */
typedef struct {
    const char *str;
    uint32_t expected_len;
} strlen_test_data_t;

static strlen_test_data_t strlen_tests[] = {
    {"hello", 5},
    {"", 0},
    {"a", 1},
    {"test string", 11}
};

TEST_CASE_PARAMETERIZED(test_string_length, strlen_test_data_t, strlen_tests)
{
    strlen_test_data_t *data = (strlen_test_data_t *)param;
    uint32_t result = OSAL_Strlen(data->str);

    TEST_PARAM_INFO("Testing strlen(\"%s\") = %u", data->str, data->expected_len);
    TEST_ASSERT_UINT_EQUAL(data->expected_len, result);
}

/* 示例 3：使用 CONTINUE 版本，即使失败也继续测试 */
typedef struct {
    int32_t value;
    bool should_be_positive;
} range_test_data_t;

static range_test_data_t range_tests[] = {
    {10, true},
    {-5, false},
    {0, false},
    {100, true},
    {-1, false}
};

TEST_CASE_PARAMETERIZED_CONTINUE(test_value_range, range_test_data_t, range_tests)
{
    range_test_data_t *data = (range_test_data_t *)param;
    bool is_positive = (data->value > 0);

    TEST_PARAM_INFO("Testing value=%d, expected_positive=%d",
                    data->value, data->should_be_positive);

    if (data->should_be_positive) {
        TEST_ASSERT_TRUE(is_positive);
    } else {
        TEST_ASSERT_FALSE(is_positive);
    }
}

/* 注册测试模块 */
TEST_MODULE_BEGIN(parameterized_example, "EXAMPLE")
    TEST_CASE_REF(test_addition)
    TEST_CASE_REF(test_string_length)
    TEST_CASE_REF(test_value_range)
TEST_MODULE_END(parameterized_example, "EXAMPLE")
