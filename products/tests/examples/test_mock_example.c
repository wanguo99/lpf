/**
 * @file test_mock_example.c
 * @brief Mock 框架使用示例
 *
 * 演示如何使用 Mock 功能进行隔离测试
 */

#include <stdint.h>
#include "test_framework.h"
#include "test_mock.h"
#include "osal/osal.h"

/* ============================================================================
 * 示例 1：Mock 外部函数以隔离测试
 * ============================================================================ */

/* 假设这是一个需要硬件的外部函数 */
int32_t hardware_read_sensor(uint32_t sensor_id, int32_t *value);

/* 我们要测试的业务逻辑函数 */
static int32_t process_sensor_data(uint32_t sensor_id)
{
    int32_t value;
    int32_t ret;

    ret = hardware_read_sensor(sensor_id, &value);
    if (ret != OSAL_SUCCESS)
    {
        return ret;
    }

    /* 业务逻辑：传感器值必须在合理范围内 */
    if (value < 0 || value > 100)
    {
        return OSAL_ERR_GENERIC;
    }

    return OSAL_SUCCESS;
}

/* Mock 函数：模拟硬件读取 */
int32_t mock_hardware_read_sensor(uint32_t sensor_id, int32_t *value)
{
    TEST_MOCK_CALLED();

    /* 只捕获指针参数，避免整数转指针警告 */
    test_mock_state_t *mock = test_mock_get(__func__);
    if (mock)
    {
        mock->last_arg1 = value;  /* 只记录指针参数 */
    }

    /* 从 mock 状态获取应该返回的值 */
    if (mock && mock->return_value_set && value)
    {
        *value = mock->return_value;
    }

    return TEST_MOCK_RETURN_INT();
}

/* 真实的硬件函数（弱符号，可被 mock 替换） */
TEST_MOCKABLE
int32_t hardware_read_sensor(uint32_t sensor_id, int32_t *value)
{
    (void)sensor_id;
    (void)value;
    /* 真实实现需要硬件 */
    return OSAL_ERR_GENERIC;
}

TEST_CASE(test_sensor_processing_success)
{
    test_mock_state_t *mock;
    int32_t ret;

    /* 初始化 mock 系统 */
    test_mock_init();

    /* 注册 mock */
    mock = test_mock_register("mock_hardware_read_sensor");
    TEST_ASSERT_NOT_NULL(mock);

    /* 设置 mock 行为：返回成功，传感器值为 50 */
    mock->return_value = 50;  /* 传感器值 */
    mock->return_value_set = true;

    /* 执行测试 */
    ret = process_sensor_data(1);

    /* 验证结果 */
    TEST_ASSERT_INT_EQUAL(OSAL_SUCCESS, ret);

    /* 清理 */
    test_mock_reset_all();
}

TEST_CASE(test_sensor_processing_out_of_range)
{
    test_mock_state_t *mock;
    int32_t ret;

    test_mock_init();

    mock = test_mock_register("mock_hardware_read_sensor");
    TEST_ASSERT_NOT_NULL(mock);

    /* 设置 mock 行为：返回超出范围的值 */
    mock->return_value = 150;  /* 超出范围 */
    mock->return_value_set = true;

    /* 执行测试 */
    ret = process_sensor_data(1);

    /* 验证：应该返回错误 */
    TEST_ASSERT_INT_EQUAL(OSAL_ERR_GENERIC, ret);

    test_mock_reset_all();
}

/* ============================================================================
 * 示例 2：验证函数调用
 * ============================================================================ */

/* 模拟日志函数 */
void mock_log_message(const char *msg)
{
    TEST_MOCK_CALLED();
    test_mock_state_t *mock = test_mock_get(__func__);
    if (mock)
    {
        mock->last_arg1 = (void*)msg;
    }
}

/* 业务函数：出错时记录日志 */
static int32_t function_with_logging(int32_t value)
{
    if (value < 0)
    {
        mock_log_message("Error: negative value");
        return OSAL_ERR_GENERIC;
    }
    return OSAL_SUCCESS;
}

TEST_CASE(test_error_logging)
{
    test_mock_state_t *mock;
    int32_t ret;

    test_mock_init();

    /* 注册 mock */
    mock = test_mock_register("mock_log_message");
    TEST_ASSERT_NOT_NULL(mock);

    /* 执行：传入负值 */
    ret = function_with_logging(-5);

    /* 验证：函数应该返回错误 */
    TEST_ASSERT_INT_EQUAL(OSAL_ERR_GENERIC, ret);

    /* 验证：日志函数应该被调用 */
    TEST_MOCK_VERIFY_CALLED(mock_log_message);
    TEST_MOCK_VERIFY_CALL_COUNT(mock_log_message, 1);

    test_mock_reset_all();
}

TEST_CASE(test_no_error_no_logging)
{
    test_mock_state_t *mock;
    int32_t ret;

    test_mock_init();

    mock = test_mock_register("mock_log_message");
    TEST_ASSERT_NOT_NULL(mock);

    /* 执行：传入正值 */
    ret = function_with_logging(10);

    /* 验证：函数应该成功 */
    TEST_ASSERT_INT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证：日志函数不应该被调用 */
    TEST_MOCK_VERIFY_NOT_CALLED(mock_log_message);

    test_mock_reset_all();
}

/* ============================================================================
 * 示例 3：参数捕获和验证
 * ============================================================================ */

/* Mock 函数，捕获参数 */
int32_t mock_write_register(uint32_t addr, uint32_t value)
{
    TEST_MOCK_CALLED();
    /* 避免整数转指针警告，使用 uintptr_t */
    test_mock_state_t *mock = test_mock_get(__func__);
    if (mock)
    {
        mock->last_arg1 = (void*)(uintptr_t)addr;
        mock->last_arg2 = (void*)(uintptr_t)value;
    }
    return TEST_MOCK_RETURN_INT();
}

TEST_CASE(test_parameter_capture)
{
    test_mock_state_t *mock;

    test_mock_init();

    mock = test_mock_register("mock_write_register");
    TEST_ASSERT_NOT_NULL(mock);

    /* 设置返回值 */
    mock->return_value = OSAL_SUCCESS;
    mock->return_value_set = true;

    /* 调用 mock 函数 */
    mock_write_register(0x1000, 0xABCD);

    /* 验证调用和参数 */
    TEST_MOCK_VERIFY_CALLED(mock_write_register);
    TEST_ASSERT_PTR_EQUAL((void*)0x1000, mock->last_arg1);
    TEST_ASSERT_PTR_EQUAL((void*)0xABCD, mock->last_arg2);

    test_mock_reset_all();
}

/* 注册测试模块 */
TEST_MODULE_BEGIN(mock_example, "EXAMPLE")
    TEST_CASE_REF(test_sensor_processing_success)
    TEST_CASE_REF(test_sensor_processing_out_of_range)
    TEST_CASE_REF(test_error_logging)
    TEST_CASE_REF(test_no_error_no_logging)
    TEST_CASE_REF(test_parameter_capture)
TEST_MODULE_END(mock_example, "EXAMPLE")
