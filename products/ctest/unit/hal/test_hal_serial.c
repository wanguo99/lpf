#include <test_framework/test_framework.h>
/**
 * @file test_hal_serial.c
 * @brief HAL串口驱动单元测试
 */

#include "hal.h"
#include "osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: 串口打开 - 成功 */
static void _test_hal_serial_open_success(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_NOT_NULL(handle);

	hal_serial_close(handle);
}

/* 测试用例: 串口打开 - 空设备路径 */
static void _test_hal_serial_open_null_device(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open(NULL, &config, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口打开 - 空配置 */
static void _test_hal_serial_open_null_config(void)
{
	hal_serial_handle_t handle = NULL;

	int32_t ret = hal_serial_open("/dev/ttyS0", NULL, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口打开 - 空句柄 */
static void _test_hal_serial_open_null_handle(void)
{
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口打开 - 无效设备 */
static void _test_hal_serial_open_invalid_device(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/invalid_tty999", &config, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口关闭 */
static void _test_hal_serial_close(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_close(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口关闭 - 空句柄 */
static void _test_hal_serial_close_null_handle(void)
{
	int32_t ret = hal_serial_close(NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 读写测试
 *===========================================================================*/

/* 测试用例: 串口写入 - 成功 */
static void _test_hal_serial_write_success(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };
	uint8_t data[] = "Hello Serial";

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_write(handle, data, OSAL_sizeof(data), 1000);
	TEST_ASSERT_TRUE(ret > 0);

	hal_serial_close(handle);
}

/* 测试用例: 串口写入 - 空句柄 */
static void _test_hal_serial_write_null_handle(void)
{
	uint8_t data[] = "test";

	int32_t ret = hal_serial_write(NULL, data, OSAL_sizeof(data), 1000);
	TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例: 串口写入 - 空缓冲区 */
static void _test_hal_serial_write_null_buffer(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_write(handle, NULL, 10, 1000);
	TEST_ASSERT_TRUE(ret < 0);

	hal_serial_close(handle);
}

/* 测试用例: 串口写入 - 零长度 */
static void _test_hal_serial_write_zero_length(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };
	uint8_t data[] = "test";

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_write(handle, data, 0, 1000);
	TEST_ASSERT_EQUAL(0, ret);

	hal_serial_close(handle);
}

/* 测试用例: 串口读取 - 超时 */
static void _test_hal_serial_read_timeout(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };
	uint8_t buffer[64];

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_read(handle, buffer, OSAL_sizeof(buffer), 100);
	/* 超时或读取到数据都是正常的 */

	hal_serial_close(handle);
}

/* 测试用例: 串口读取 - 空句柄 */
static void _test_hal_serial_read_null_handle(void)
{
	uint8_t buffer[64];

	int32_t ret = hal_serial_read(NULL, buffer, OSAL_sizeof(buffer), 100);
	TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例: 串口读取 - 空缓冲区 */
static void _test_hal_serial_read_null_buffer(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_read(handle, NULL, 64, 100);
	TEST_ASSERT_TRUE(ret < 0);

	hal_serial_close(handle);
}

/*===========================================================================
 * 刷新测试
 *===========================================================================*/

/* 测试用例: 刷新缓冲区 - 成功 */
static void _test_hal_serial_flush_success(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_flush(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_serial_close(handle);
}

/* 测试用例: 刷新缓冲区 - 空句柄 */
static void _test_hal_serial_flush_null_handle(void)
{
	int32_t ret = hal_serial_flush(NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 配置测试
 *===========================================================================*/

/* 测试用例: 设置配置 - 成功 */
static void _test_hal_serial_set_config_success(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };
	hal_serial_config_t new_config = { .baud_rate = 9600,
									   .data_bits = 8,
									   .stop_bits = 1,
									   .parity = HAL_SERIAL_PARITY_EVEN,
									   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_set_config(handle, &new_config);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_serial_close(handle);
}

/* 测试用例: 设置配置 - 空句柄 */
static void _test_hal_serial_set_config_null_handle(void)
{
	hal_serial_config_t config = { .baud_rate = 9600,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_set_config(NULL, &config);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 设置配置 - 空配置 */
static void _test_hal_serial_set_config_null_config(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	ret = hal_serial_set_config(handle, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_serial_close(handle);
}

/*===========================================================================
 * 配置参数测试
 *===========================================================================*/

/* 测试用例: 不同波特率 */
static void _test_hal_serial_different_baudrate(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 9600,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_serial_close(handle);
}

/* 测试用例: 不同校验位 */
static void _test_hal_serial_different_parity(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_ODD,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_serial_close(handle);
}

/* 测试用例: 不同数据位 */
static void _test_hal_serial_different_databits(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 7,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_serial_close(handle);
}

/* 测试用例: 不同停止位 */
static void _test_hal_serial_different_stopbits(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 2,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_serial_close(handle);

	/* 测试1个停止位 */
	config.stop_bits = 1;
	ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_serial_close(handle);
}

/* 测试用例: 硬件流控 */
static void _test_hal_serial_hardware_flow_control(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_HW };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_MESSAGE("SKIPPED: Hardware flow control not available");
		return;
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 尝试写入数据 */
	uint8_t data[] = "Test HW Flow";
	ret = hal_serial_write(handle, data, sizeof(data), 1000);
	TEST_ASSERT_TRUE(ret >= 0);

	hal_serial_close(handle);
}

/* 测试用例: 软件流控 */
static void _test_hal_serial_software_flow_control(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_SW };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_MESSAGE("SKIPPED: Software flow control not available");
		return;
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	/* 尝试写入数据 */
	uint8_t data[] = "Test SW Flow";
	ret = hal_serial_write(handle, data, sizeof(data), 1000);
	TEST_ASSERT_TRUE(ret >= 0);

	hal_serial_close(handle);
}

/* 测试用例: 边界值测试 - 最大波特率 */
static void _test_hal_serial_max_baudrate(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 921600,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_MESSAGE("SKIPPED: Max baud rate not supported");
		return;
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_serial_close(handle);
}

/* 测试用例: 边界值测试 - 最小波特率 */
static void _test_hal_serial_min_baudrate(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 300,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_MESSAGE("SKIPPED: Min baud rate not supported");
		return;
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_serial_close(handle);
}

/* 测试用例: 大数据写入 */
static void _test_hal_serial_large_write(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	/* 写入1KB数据 */
	uint8_t large_data[1024];
	for (uint32_t i = 0; i < sizeof(large_data); i++) {
		large_data[i] = (uint8_t)(i & 0xFF);
	}

	ret = hal_serial_write(handle, large_data, sizeof(large_data), 5000);
	TEST_ASSERT_TRUE(ret > 0);

	hal_serial_close(handle);
}

/* 测试用例: 非阻塞读取 */
static void _test_hal_serial_nonblocking_read(void)
{
	hal_serial_handle_t handle = NULL;
	hal_serial_config_t config = { .baud_rate = 115200,
								   .data_bits = 8,
								   .stop_bits = 1,
								   .parity = HAL_SERIAL_PARITY_NONE,
								   .flow_control = HAL_SERIAL_FLOW_NONE };
	uint8_t buffer[64];

	int32_t ret = hal_serial_open("/dev/ttyS0", &config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/ttyS0 not available
	}

	/* 非阻塞读取 (timeout = 0) */
	ret = hal_serial_read(handle, buffer, sizeof(buffer), 0);
	/* 应该立即返回，可能返回0或负数 */
	TEST_ASSERT_TRUE(ret >= 0 || ret == OSAL_ERR_TIMEOUT);

	hal_serial_close(handle);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_hal_serial_open_success",
	  .func = _test_hal_serial_open_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_open_null_device",
	  .func = _test_hal_serial_open_null_device,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_open_null_config",
	  .func = _test_hal_serial_open_null_config,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_open_null_handle",
	  .func = _test_hal_serial_open_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_open_invalid_device",
	  .func = _test_hal_serial_open_invalid_device,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_close",
	  .func = _test_hal_serial_close,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_close_null_handle",
	  .func = _test_hal_serial_close_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_write_success",
	  .func = _test_hal_serial_write_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_write_null_handle",
	  .func = _test_hal_serial_write_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_write_null_buffer",
	  .func = _test_hal_serial_write_null_buffer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_write_zero_length",
	  .func = _test_hal_serial_write_zero_length,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_read_timeout",
	  .func = _test_hal_serial_read_timeout,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_read_null_handle",
	  .func = _test_hal_serial_read_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_read_null_buffer",
	  .func = _test_hal_serial_read_null_buffer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_flush_success",
	  .func = _test_hal_serial_flush_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_flush_null_handle",
	  .func = _test_hal_serial_flush_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_set_config_success",
	  .func = _test_hal_serial_set_config_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_set_config_null_handle",
	  .func = _test_hal_serial_set_config_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_set_config_null_config",
	  .func = _test_hal_serial_set_config_null_config,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_different_baudrate",
	  .func = _test_hal_serial_different_baudrate,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_different_parity",
	  .func = _test_hal_serial_different_parity,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_different_databits",
	  .func = _test_hal_serial_different_databits,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_different_stopbits",
	  .func = _test_hal_serial_different_stopbits,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_hardware_flow_control",
	  .func = _test_hal_serial_hardware_flow_control,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_software_flow_control",
	  .func = _test_hal_serial_software_flow_control,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_max_baudrate",
	  .func = _test_hal_serial_max_baudrate,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_min_baudrate",
	  .func = _test_hal_serial_min_baudrate,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_large_write",
	  .func = _test_hal_serial_large_write,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_serial_nonblocking_read",
	  .func = _test_hal_serial_nonblocking_read,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "hal_serial",
	.module_name = "hal_serial",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "HAL hal_serial tests" }
};

/* 测试套件注册函数 */
void register_hal_serial_tests(void)
{
	libutest_register_suite(&test_suite);
}
