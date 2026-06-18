/**
 * @file test_mock.h
 * @brief 函数 Mock 支持框架
 *
 * 提供函数替换和调用验证功能，用于隔离测试和依赖注入。
 *
 * 功能特性：
 * 1. 函数替换 - 用 mock 函数替换真实函数
 * 2. 调用验证 - 验证函数是否被调用以及调用次数
 * 3. 参数捕获 - 记录函数调用的参数
 * 4. 返回值控制 - 设置 mock 函数的返回值
 *
 * 使用场景：
 * - 测试需要硬件支持的代码（HAL 层）
 * - 测试错误处理逻辑（模拟失败情况）
 * - 隔离外部依赖
 *
 * 使用示例：
 * @code
 * // 1. 定义 mock 函数
 * int32_t mock_HAL_CAN_init(const hal_can_config_t *cfg, hal_can_handle_t *hdl)
 * { TEST_MOCK_CALLED();  // 记录被调用 *hdl = (hal_can_handle_t)0x12345678;  //
 * 返回假句柄 return TEST_MOCK_RETURN_INT();  // 返回预设值
 * }
 *
 * // 2. 在测试中使用
 * TEST_CASE(test_pdl_without_hardware) {
 *     // 替换真实函数为 mock
 *     TEST_MOCK_REPLACE(HAL_CAN_Init, mock_HAL_CAN_Init);
 *
 *     // 设置 mock 返回值
 *     TEST_MOCK_SET_RETURN(OSAL_SUCCESS);
 *
 *     // 执行测试
 *     int32_t ret = pdl_mcu_init(&config);
 *
 *     // 验证 mock 被调用
 *     TEST_MOCK_VERIFY_CALLED(HAL_CAN_Init);
 *     TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
 *
 *     // 恢复真实函数
 *     TEST_MOCK_RESTORE_ALL();
 * }
 * @endcode
 *
 * 限制：
 * - 仅支持链接时替换（需要编译时支持）
 * - 不支持内联函数和静态函数
 * - Mock 函数签名必须与原函数完全一致
 */

#ifndef TEST_MOCK_H
#define TEST_MOCK_H

#include <stdint.h>
#include <stdbool.h>
#include "test_assert.h"

/* 注意：源文件需要包含 osal.h 以使用 OSAL API */

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_MOCK_MAX_FUNCS 32

/*
 * 声明弱符号，允许 mock 替换。
 */
#ifdef __GNUC__
#define TEST_MOCKABLE __attribute__((weak))
#else
#define TEST_MOCKABLE
#endif

/* Mock 状态跟踪 */
typedef struct {
	const char *func_name; /* 函数名称 */
	uint32_t call_count; /* 调用次数 */
	int32_t return_value; /* 返回值（整数） */
	void *return_ptr; /* 返回值（指针） */
	bool return_value_set; /* 是否设置了返回值 */

	/* 最后一次调用的参数（通用存储） */
	void *last_arg1;
	void *last_arg2;
	void *last_arg3;
	void *last_arg4;
} test_mock_state_t;

/* Mock 注册表 */
typedef struct {
	test_mock_state_t mocks[TEST_MOCK_MAX_FUNCS];
	uint32_t mock_count;
} test_mock_registry_t;

/**
 * @brief 记录函数被调用
 */
#define TEST_MOCK_CALLED()                                  \
	do {                                                    \
		test_mock_state_t *_mock = test_mock_get(__func__); \
		if (_mock) {                                        \
			_mock->call_count++;                            \
		}                                                   \
	} while (0)

/**
 * @brief 设置 mock 函数的整数返回值
 *
 * @param value 返回值
 */
#define TEST_MOCK_SET_RETURN(value)                               \
	do {                                                          \
		test_mock_state_t *_mock = test_mock_get(g_current_test); \
		if (_mock) {                                              \
			_mock->return_value = (value);                        \
			_mock->return_value_set = true;                       \
		}                                                         \
	} while (0)

/**
 * @brief 设置 mock 函数的指针返回值
 *
 * @param ptr 指针返回值
 */
#define TEST_MOCK_SET_RETURN_PTR(ptr)                             \
	do {                                                          \
		test_mock_state_t *_mock = test_mock_get(g_current_test); \
		if (_mock) {                                              \
			_mock->return_ptr = (void *)(ptr);                    \
			_mock->return_value_set = true;                       \
		}                                                         \
	} while (0)

/**
 * @brief 获取 mock 函数应该返回的整数值
 *
 * @return 预设的返回值，如果未设置则返回 0
 */
#define TEST_MOCK_RETURN_INT()                                        \
	({                                                                \
		test_mock_state_t *_mock = test_mock_get(__func__);           \
		(_mock && _mock->return_value_set) ? _mock->return_value : 0; \
	})

/**
 * @brief 获取 mock 函数应该返回的指针值
 *
 * @return 预设的指针，如果未设置则返回 NULL
 */
#define TEST_MOCK_RETURN_PTR()                                         \
	({                                                                 \
		test_mock_state_t *_mock = test_mock_get(__func__);            \
		(_mock && _mock->return_value_set) ? _mock->return_ptr : NULL; \
	})

/**
 * @brief 验证函数被调用
 *
 * @param func_name 函数名称（不带引号）
 */
#define TEST_MOCK_VERIFY_CALLED(func_name)                                   \
	do {                                                                     \
		test_mock_state_t *_mock = test_mock_get(#func_name);                \
		if (!_mock || _mock->call_count == 0) {                              \
			osal_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__);         \
			osal_printf("            Expected function '%s' to be called\n", \
						#func_name);                                         \
			osal_printf("            Actual call count: %u\n",               \
						_mock ? _mock->call_count : 0);                      \
			g_test_failed = true;                                            \
			return;                                                          \
		}                                                                    \
	} while (0)

/**
 * @brief 验证函数被调用指定次数
 *
 * @param func_name 函数名称（不带引号）
 * @param expected_count 期望的调用次数
 */
#define TEST_MOCK_VERIFY_CALL_COUNT(func_name, expected_count)             \
	do {                                                                   \
		test_mock_state_t *_mock = test_mock_get(#func_name);              \
		uint32_t _actual = _mock ? _mock->call_count : 0;                  \
		if (_actual != (expected_count)) {                                 \
			osal_printf("[  FAILED  ] %s:%d\n", __FILE__, __LINE__);       \
			osal_printf("            Function: %s\n", #func_name);         \
			osal_printf("            Expected call count: %u\n",           \
						(uint32_t)(expected_count));                       \
			osal_printf("            Actual call count:   %u\n", _actual); \
			g_test_failed = true;                                          \
			return;                                                        \
		}                                                                  \
	} while (0)

/**
 * @brief 验证函数未被调用
 *
 * @param func_name 函数名称（不带引号）
 */
#define TEST_MOCK_VERIFY_NOT_CALLED(func_name) \
	TEST_MOCK_VERIFY_CALL_COUNT(func_name, 0)

/**
 * @brief 捕获函数调用的参数（用于 mock 函数内部）
 *
 * @param arg1 第一个参数
 * @param arg2 第二个参数（可选）
 * @param arg3 第三个参数（可选）
 * @param arg4 第四个参数（可选）
 */
#define TEST_MOCK_CAPTURE_ARGS(arg1, arg2, arg3, arg4)      \
	do {                                                    \
		test_mock_state_t *_mock = test_mock_get(__func__); \
		if (_mock) {                                        \
			_mock->last_arg1 = (void *)(arg1);              \
			_mock->last_arg2 = (void *)(arg2);              \
			_mock->last_arg3 = (void *)(arg3);              \
			_mock->last_arg4 = (void *)(arg4);              \
		}                                                   \
	} while (0)

/**
 * @brief 恢复所有 mock 函数
 *
 * 注意：这是一个简化版本，真正的函数替换需要链接器支持
 */
#define TEST_MOCK_RESTORE_ALL() test_mock_reset_all()

/**
 * @brief 简化的 mock 替换宏（需要链接器支持）
 *
 * 注意：这是一个占位符宏，实际的函数替换需要在编译时使用弱符号或链接器脚本
 */
#define TEST_MOCK_REPLACE(original_func, mock_func)             \
	do {                                                        \
		test_mock_register(#mock_func);                         \
		/* 实际的函数替换需要编译时支持 */        \
		osal_printf("[   INFO   ] Mock registered: %s -> %s\n", \
					#original_func, #mock_func);                \
	} while (0)

/* 全局 mock 注册表 */
extern test_mock_registry_t g_mock_registry;

/**
 * @brief 初始化 mock 系统
 */
void test_mock_init(void);

/**
 * @brief 重置所有 mock 状态
 */
void test_mock_reset_all(void);

/**
 * @brief 注册一个 mock 函数
 *
 * @param func_name 函数名称
 * @return mock 状态指针
 */
test_mock_state_t *test_mock_register(const char *func_name);

/**
 * @brief 获取 mock 状态
 *
 * @param func_name 函数名称
 * @return mock 状态指针，如果未找到则返回 NULL
 */
test_mock_state_t *test_mock_get(const char *func_name);

#ifdef __cplusplus
}
#endif

#endif /* TEST_MOCK_H */
