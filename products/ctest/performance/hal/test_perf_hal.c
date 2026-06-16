/**
 * @file test_perf_hal.c
 * @brief HAL层性能测试
 */

#include <test_framework/test_framework.h>
#include <test_framework/test_performance.h>
#include "hal.h"

/**
 * 测试CAN发送性能
 */
static void test_perf_can_send(void) {
    // TODO: 实现CAN发送性能测试
    OSAL_printf("[ INFO ] CAN send performance test - not implemented yet\n");
}

/**
 * 测试UART发送性能
 */
static void test_perf_uart_send(void) {
    // TODO: 实现UART发送性能测试
    OSAL_printf("[ INFO ] UART send performance test - not implemented yet\n");
}

/**
 * 测试GPIO读写性能
 */
static void test_perf_gpio_read_write(void) {
    // TODO: 实现GPIO读写性能测试
    OSAL_printf("[ INFO ] GPIO read/write performance test - not implemented yet\n");
}

/**
 * 测试I2C传输性能
 */
static void test_perf_i2c_transfer(void) {
    // TODO: 实现I2C传输性能测试
    OSAL_printf("[ INFO ] I2C transfer performance test - not implemented yet\n");
}

/**
 * 测试SPI传输性能
 */
static void test_perf_spi_transfer(void) {
    // TODO: 实现SPI传输性能测试
    OSAL_printf("[ INFO ] SPI transfer performance test - not implemented yet\n");
}

/* 注册性能测试模块 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{
		.name = "test_perf_can_send",
		.func = test_perf_can_send,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_perf_uart_send",
		.func = test_perf_uart_send,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_perf_gpio_read_write",
		.func = test_perf_gpio_read_write,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_perf_i2c_transfer",
		.func = test_perf_i2c_transfer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_perf_spi_transfer",
		.func = test_perf_spi_transfer,
		.setup = NULL,
		.teardown = NULL
	},
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "perf_hal",
	.module_name = "perf_hal",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = {
		.category = TEST_CATEGORY_PERFORMANCE,
		.tags = TEST_TAG_SLOW,
		.timeout_ms = 5000,
		.description = "HAL perf_hal tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_perf_hal_tests(void)
{
	libutest_register_suite(&test_suite);
}
