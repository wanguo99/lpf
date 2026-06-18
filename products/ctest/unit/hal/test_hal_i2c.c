#include <test_framework/test_framework.h>
/**
 * @file test_hal_i2c.c
 * @brief HAL I2C驱动单元测试
 */

#include "hal.h"
#include "osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: I2C打开 - 成功 */
static void _test_hal_i2c_open_success(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);

	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/i2c-0 not available
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_NOT_NULL(handle);

	hal_i2c_close(handle);
}

/* 测试用例: I2C打开 - 空配置 */
static void _test_hal_i2c_open_null_config(void)
{
	hal_i2c_handle_t handle = NULL;

	int32_t ret = hal_i2c_open(NULL, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C打开 - 空句柄 */
static void _test_hal_i2c_open_null_handle(void)
{
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C打开 - 无效设备 */
static void _test_hal_i2c_open_invalid_device(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-999", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C关闭 */
static void _test_hal_i2c_close(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/i2c-0 not available
	}

	ret = hal_i2c_close(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C关闭 - 空句柄 */
static void _test_hal_i2c_close_null_handle(void)
{
	int32_t ret = hal_i2c_close(NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 读写操作测试
 *===========================================================================*/

/* 测试用例: I2C写入 - 空句柄 */
static void _test_hal_i2c_write_null_handle(void)
{
	uint8_t buffer[4] = { 0x01, 0x02, 0x03, 0x04 };

	int32_t ret = hal_i2c_write(NULL, 0x50, buffer, OSAL_sizeof(buffer));
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C写入 - 空缓冲区 */
static void _test_hal_i2c_write_null_buffer(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/i2c-0 not available
	}

	ret = hal_i2c_write(handle, 0x50, NULL, 4);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_i2c_close(handle);
}

/* 测试用例: I2C读取 - 空句柄 */
static void _test_hal_i2c_read_null_handle(void)
{
	uint8_t buffer[4];

	int32_t ret = hal_i2c_read(NULL, 0x50, buffer, OSAL_sizeof(buffer));
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C读取 - 空缓冲区 */
static void _test_hal_i2c_read_null_buffer(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/i2c-0 not available
	}

	ret = hal_i2c_read(handle, 0x50, NULL, 4);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_i2c_close(handle);
}

/*===========================================================================
 * 寄存器操作测试
 *===========================================================================*/

/* 测试用例: I2C写寄存器 - 空句柄 */
static void _test_hal_i2c_write_reg_null_handle(void)
{
	uint8_t buffer[4] = { 0x01, 0x02, 0x03, 0x04 };

	int32_t ret =
		hal_i2c_write_reg(NULL, 0x50, 0x00, buffer, OSAL_sizeof(buffer));
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C写寄存器 - 空缓冲区 */
static void _test_hal_i2c_write_reg_null_buffer(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/i2c-0 not available
	}

	ret = hal_i2c_write_reg(handle, 0x50, 0x00, NULL, 4);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_i2c_close(handle);
}

/* 测试用例: I2C读寄存器 - 空句柄 */
static void _test_hal_i2c_read_reg_null_handle(void)
{
	uint8_t buffer[4];

	int32_t ret =
		hal_i2c_read_reg(NULL, 0x50, 0x00, buffer, OSAL_sizeof(buffer));
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C读寄存器 - 空缓冲区 */
static void _test_hal_i2c_read_reg_null_buffer(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/i2c-0 not available
	}

	ret = hal_i2c_read_reg(handle, 0x50, 0x00, NULL, 4);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_i2c_close(handle);
}

/*===========================================================================
 * 传输操作测试
 *===========================================================================*/

/* 测试用例: I2C传输 - 空句柄 */
static void _test_hal_i2c_transfer_null_handle(void)
{
	uint8_t buffer[4] = { 0x01, 0x02, 0x03, 0x04 };
	hal_i2c_msg_t msg = {
		.addr = 0x50, .flags = 0, .len = OSAL_sizeof(buffer), .buf = buffer
	};

	int32_t ret = hal_i2c_transfer(NULL, &msg, 1);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C传输 - 空消息 */
static void _test_hal_i2c_transfer_null_msgs(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/i2c-0 not available
	}

	ret = hal_i2c_transfer(handle, NULL, 1);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_i2c_close(handle);
}

/*===========================================================================
 * 边界值和功能测试
 *===========================================================================*/

/* 测试用例: I2C最小从机地址 */
static void _test_hal_i2c_min_slave_address(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true);
	}

	/* 地址0x08是最小有效从机地址 */
	uint8_t data = 0xAA;
	ret = hal_i2c_write(handle, 0x08, &data, 1);
	/* 可能失败（设备不存在），但不应该崩溃 */

	hal_i2c_close(handle);
}

/* 测试用例: I2C最大从机地址 */
static void _test_hal_i2c_max_slave_address(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true);
	}

	/* 地址0x77是最大7位从机地址 */
	uint8_t data = 0x55;
	ret = hal_i2c_write(handle, 0x77, &data, 1);
	/* 可能失败（设备不存在），但不应该崩溃 */

	hal_i2c_close(handle);
}

/* 测试用例: I2C大数据写入 */
static void _test_hal_i2c_large_write(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 5000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true);
	}

	/* 写入256字节数据 */
	uint8_t large_data[256];
	for (uint32_t i = 0; i < sizeof(large_data); i++) {
		large_data[i] = (uint8_t)(i & 0xFF);
	}

	ret = hal_i2c_write(handle, 0x50, large_data, sizeof(large_data));
	/* 可能失败（设备不存在或不支持），但不应该崩溃 */

	hal_i2c_close(handle);
}

/* 测试用例: I2C大数据读取 */
static void _test_hal_i2c_large_read(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 5000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true);
	}

	/* 读取256字节数据 */
	uint8_t rx_buffer[256];
	ret = hal_i2c_read(handle, 0x50, rx_buffer, sizeof(rx_buffer));
	/* 可能失败（设备不存在），但不应该崩溃 */

	hal_i2c_close(handle);
}

/* 测试用例: I2C零长度传输 */
static void _test_hal_i2c_zero_length_transfer(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true);
	}

	uint8_t buffer[4];

	/* 零长度写入 */
	ret = hal_i2c_write(handle, 0x50, buffer, 0);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	/* 零长度读取 */
	ret = hal_i2c_read(handle, 0x50, buffer, 0);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_i2c_close(handle);
}

/* 测试用例: I2C寄存器连续写入 */
static void _test_hal_i2c_register_burst_write(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true);
	}

	/* 连续写入多个寄存器 */
	for (uint8_t reg = 0x00; reg < 0x10; reg++) {
		uint8_t data = reg * 2;
		ret = hal_i2c_write_reg(handle, 0x50, reg, &data, 1);
		/* 可能失败，但不应该崩溃 */
	}

	hal_i2c_close(handle);
}

/* 测试用例: I2C寄存器连续读取 */
static void _test_hal_i2c_register_burst_read(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true);
	}

	/* 连续读取多个寄存器 */
	for (uint8_t reg = 0x00; reg < 0x10; reg++) {
		uint8_t data;
		ret = hal_i2c_read_reg(handle, 0x50, reg, &data, 1);
		/* 可能失败，但不应该崩溃 */
	}

	hal_i2c_close(handle);
}

/* 测试用例: I2C多消息传输 */
static void _test_hal_i2c_multi_message_transfer(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = { .device = "/dev/i2c-0", .timeout = 1000 };

	int32_t ret = hal_i2c_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true);
	}

	/* 准备多个消息 */
	uint8_t tx_buf[4] = { 0x01, 0x02, 0x03, 0x04 };
	uint8_t rx_buf[4];

	hal_i2c_msg_t msgs[2] = { { .addr = 0x50,
								.flags = 0, /* 写 */
								.len = 1,
								.buf = tx_buf },
							  { .addr = 0x50,
								.flags = I2C_M_RD, /* 读 */
								.len = sizeof(rx_buf),
								.buf = rx_buf } };

	ret = hal_i2c_transfer(handle, msgs, 2);
	/* 可能失败（设备不存在），但不应该崩溃 */

	hal_i2c_close(handle);
}

/* 测试用例: I2C不同超时值 */
static void _test_hal_i2c_different_timeouts(void)
{
	hal_i2c_handle_t handle = NULL;
	hal_i2c_config_t config = {
		.device = "/dev/i2c-0", .timeout = 100 /* 短超时 */
	};

	int32_t ret = hal_i2c_open(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_i2c_close(handle);
	}

	/* 长超时 */
	config.timeout = 10000;
	ret = hal_i2c_open(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_i2c_close(handle);
	}

	/* 零超时 */
	config.timeout = 0;
	ret = hal_i2c_open(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_i2c_close(handle);
	}
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 初始化和清理 */
/* 读写操作 */
/* 寄存器操作 */
/* 传输操作 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_hal_i2c_open_success",
	  .func = _test_hal_i2c_open_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_open_null_config",
	  .func = _test_hal_i2c_open_null_config,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_open_null_handle",
	  .func = _test_hal_i2c_open_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_open_invalid_device",
	  .func = _test_hal_i2c_open_invalid_device,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_close",
	  .func = _test_hal_i2c_close,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_close_null_handle",
	  .func = _test_hal_i2c_close_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_write_null_handle",
	  .func = _test_hal_i2c_write_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_write_null_buffer",
	  .func = _test_hal_i2c_write_null_buffer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_read_null_handle",
	  .func = _test_hal_i2c_read_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_read_null_buffer",
	  .func = _test_hal_i2c_read_null_buffer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_write_reg_null_handle",
	  .func = _test_hal_i2c_write_reg_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_write_reg_null_buffer",
	  .func = _test_hal_i2c_write_reg_null_buffer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_read_reg_null_handle",
	  .func = _test_hal_i2c_read_reg_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_read_reg_null_buffer",
	  .func = _test_hal_i2c_read_reg_null_buffer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_transfer_null_handle",
	  .func = _test_hal_i2c_transfer_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_transfer_null_msgs",
	  .func = _test_hal_i2c_transfer_null_msgs,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_min_slave_address",
	  .func = _test_hal_i2c_min_slave_address,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_max_slave_address",
	  .func = _test_hal_i2c_max_slave_address,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_large_write",
	  .func = _test_hal_i2c_large_write,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_large_read",
	  .func = _test_hal_i2c_large_read,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_zero_length_transfer",
	  .func = _test_hal_i2c_zero_length_transfer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_register_burst_write",
	  .func = _test_hal_i2c_register_burst_write,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_register_burst_read",
	  .func = _test_hal_i2c_register_burst_read,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_multi_message_transfer",
	  .func = _test_hal_i2c_multi_message_transfer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_i2c_different_timeouts",
	  .func = _test_hal_i2c_different_timeouts,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "hal_i2c",
	.module_name = "hal_i2c",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "HAL hal_i2c tests" }
};

/* 测试套件注册函数 */
void register_hal_i2c_tests(void)
{
	libutest_register_suite(&test_suite);
}
