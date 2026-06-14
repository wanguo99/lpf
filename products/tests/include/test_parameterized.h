/**
 * @file test_parameterized.h
 * @brief 参数化测试支持
 *
 * 允许使用同一个测试函数测试多组输入数据，减少重复代码。
 *
 * 使用示例：
 * @code
 * // 定义测试数据结构
 * typedef struct {
 *     uint32_t baudrate;
 *     int32_t expected_result;
 * } baudrate_test_data_t;
 *
 * // 定义测试数据数组
 * static baudrate_test_data_t baudrate_tests[] = {
 *     {125000, OSAL_SUCCESS},
 *     {250000, OSAL_SUCCESS},
 *     {500000, OSAL_SUCCESS},
 *     {999999, OSAL_ERR_GENERIC}  // 无效波特率
 * };
 *
 * // 定义参数化测试
 * TEST_CASE_PARAMETERIZED(test_can_baudrate_validation,
 *                        baudrate_test_data_t,
 *                        baudrate_tests)
 * {
 *     baudrate_test_data_t *data = (baudrate_test_data_t *)param;
 *     int32_t result = validate_baudrate(data->baudrate);
 *     TEST_ASSERT_INT_EQUAL(data->expected_result, result);
 * }
 * @endcode
 */

#ifndef TEST_PARAMETERIZED_H
#define TEST_PARAMETERIZED_H

#include "test_assert.h"

/**
 * @brief 定义参数化测试用例
 *
 * 测试函数会对数据数组中的每个元素执行一次。
 * 如果任何一次执行失败，整个测试用例标记为失败。
 *
 * @param test_name 测试函数名
 * @param data_type 测试数据的类型
 * @param data_array 测试数据数组
 */
#define TEST_CASE_PARAMETERIZED(test_name, data_type, data_array) \
    static void test_name##_impl(void *param); \
    static void test_name(void) { \
        size_t _test_count = OSAL_sizeof(data_array) / OSAL_sizeof(data_type); \
        size_t _test_i; \
        for (_test_i = 0; _test_i < _test_count; _test_i++) { \
            if (g_test_failed) { \
                OSAL_printf("[   INFO   ] Skipping remaining %zu parameter(s) due to previous failure\n", \
                       _test_count - _test_i); \
                return; \
            } \
            OSAL_printf("[   RUN    ] Parameter set %zu/%zu\n", _test_i + 1, _test_count); \
            test_name##_impl(&data_array[_test_i]); \
        } \
    } \
    static void test_name##_impl(void *param)

/**
 * @brief 定义参数化测试用例（继续执行版本）
 *
 * 即使某次执行失败，也会继续执行剩余的参数组合。
 * 适用于需要完整覆盖所有参数的场景。
 *
 * @param test_name 测试函数名
 * @param data_type 测试数据的类型
 * @param data_array 测试数据数组
 */
#define TEST_CASE_PARAMETERIZED_CONTINUE(test_name, data_type, data_array) \
    static void test_name##_impl(void *param); \
    static void test_name(void) { \
        size_t _test_count = OSAL_sizeof(data_array) / OSAL_sizeof(data_type); \
        size_t _test_i; \
        size_t _test_failed_count = 0; \
        for (_test_i = 0; _test_i < _test_count; _test_i++) { \
            bool _test_was_failed = g_test_failed; \
            g_test_failed = false; \
            OSAL_printf("[   RUN    ] Parameter set %zu/%zu\n", _test_i + 1, _test_count); \
            test_name##_impl(&data_array[_test_i]); \
            if (g_test_failed) { \
                _test_failed_count++; \
            } \
            g_test_failed = _test_was_failed || g_test_failed; \
        } \
        if (_test_failed_count > 0) { \
            OSAL_printf("[   INFO   ] %zu/%zu parameter set(s) failed\n", \
                   _test_failed_count, _test_count); \
        } \
    } \
    static void test_name##_impl(void *param)

/**
 * @brief 在参数化测试中跳过当前参数
 *
 * 使用示例：
 * @code
 * TEST_CASE_PARAMETERIZED(test_something, test_data_t, test_data) {
 *     test_data_t *data = (test_data_t *)param;
 *     if (data->skip_condition) {
 *         TEST_SKIP("Skipping due to condition");
 *     }
 *     // 测试代码...
 * }
 * @endcode
 */
#define TEST_SKIP(reason) \
    do { \
        OSAL_printf("[  SKIPPED ] %s\n", reason); \
        return; \
    } while(0)

/**
 * @brief 在参数化测试中输出当前参数信息
 *
 * @param format 格式化字符串
 * @param ... 参数
 */
#define TEST_PARAM_INFO(format, ...) \
    OSAL_printf("[  PARAM   ] " format "\n", ##__VA_ARGS__)

#endif /* TEST_PARAMETERIZED_H */
