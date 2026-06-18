#include <test_framework/test_framework.h>
/**
 * @file test_hal_spi.c
 * @brief HAL SPI驱动单元测试
 */

#include "hal.h"
#include "osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: SPI打开 - 成功 */
static void _test_hal_spi_open_success(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);

	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	TEST_ASSERT_NOT_NULL(handle);

	hal_spi_close(handle);
}

/* 测试用例: SPI打开 - 空配置 */
static void _test_hal_spi_open_null_config(void)
{
	hal_spi_handle_t handle = NULL;

	int32_t ret = hal_spi_open(NULL, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI打开 - 空句柄 */
static void _test_hal_spi_open_null_handle(void)
{
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI打开 - 无效设备 */
static void _test_hal_spi_open_invalid_device(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev99.99",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI关闭 */
static void _test_hal_spi_close(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	ret = hal_spi_close(handle);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI关闭 - 空句柄 */
static void _test_hal_spi_close_null_handle(void)
{
	int32_t ret = hal_spi_close(NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 读写操作测试
 *===========================================================================*/

/* 测试用例: SPI写入 - 空句柄 */
static void _test_hal_spi_write_null_handle(void)
{
	uint8_t buffer[4] = { 0x01, 0x02, 0x03, 0x04 };

	int32_t ret = hal_spi_write(NULL, buffer, OSAL_sizeof(buffer));
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI写入 - 空缓冲区 */
static void _test_hal_spi_write_null_buffer(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	ret = hal_spi_write(handle, NULL, 4);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_spi_close(handle);
}

/* 测试用例: SPI读取 - 空句柄 */
static void _test_hal_spi_read_null_handle(void)
{
	uint8_t buffer[4];

	int32_t ret = hal_spi_read(NULL, buffer, OSAL_sizeof(buffer));
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI读取 - 空缓冲区 */
static void _test_hal_spi_read_null_buffer(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	ret = hal_spi_read(handle, NULL, 4);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_spi_close(handle);
}

/*===========================================================================
 * 传输操作测试
 *===========================================================================*/

/* 测试用例: SPI全双工传输 - 空句柄 */
static void _test_hal_spi_transfer_null_handle(void)
{
	uint8_t tx_buffer[4] = { 0x01, 0x02, 0x03, 0x04 };
	uint8_t rx_buffer[4];

	int32_t ret =
		hal_spi_transfer(NULL, tx_buffer, rx_buffer, OSAL_sizeof(tx_buffer));
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI全双工传输 - 回环测试 */
static void _test_hal_spi_transfer_loopback(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };
	uint8_t tx_buffer[4] = { 0xAA, 0xBB, 0xCC, 0xDD };
	uint8_t rx_buffer[4] = { 0 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	ret =
		hal_spi_transfer(handle, tx_buffer, rx_buffer, OSAL_sizeof(tx_buffer));

	/* 注意: 需要硬件回环才能验证数据 */
	if (OSAL_SUCCESS == ret) {
		LOG_INFO("TEST",
				 "SPI transfer completed (loopback test requires hardware)");
	}

	hal_spi_close(handle);
}

/* 测试用例: SPI批量传输 - 空句柄 */
static void _test_hal_spi_transfer_multi_null_handle(void)
{
	uint8_t tx_buffer[4] = { 0x01, 0x02, 0x03, 0x04 };
	uint8_t rx_buffer[4];
	hal_spi_transfer_t xfer = { .tx_buf = tx_buffer,
								.rx_buf = rx_buffer,
								.len = OSAL_sizeof(tx_buffer),
								.speed_hz = 0,
								.delay_usecs = 0,
								.bits_per_word = 0,
								.cs_change = 0 };

	int32_t ret = hal_spi_transfer_multi(NULL, &xfer, 1);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI批量传输 - 空传输数组 */
static void _test_hal_spi_transfer_multi_null_transfers(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	ret = hal_spi_transfer_multi(handle, NULL, 1);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_spi_close(handle);
}

/*===========================================================================
 * 配置操作测试
 *===========================================================================*/

/* 测试用例: SPI设置配置 - 空句柄 */
static void _test_hal_spi_set_config_null_handle(void)
{
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_1,
								.bits_per_word = 8,
								.max_speed_hz = 500000,
								.timeout = 1000 };

	int32_t ret = hal_spi_set_config(NULL, &config);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI设置配置 - 空配置 */
static void _test_hal_spi_set_config_null_config(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	ret = hal_spi_set_config(handle, NULL);
	TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

	hal_spi_close(handle);
}

/* 测试用例: SPI设置配置 - 更改模式 */
static void _test_hal_spi_set_config_change_mode(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	/* 更改为MODE_1 */
	config.mode = SPI_MODE_1;
	ret = hal_spi_set_config(handle, &config);
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_spi_close(handle);
}

/*===========================================================================
 * 独立功能测试
 *===========================================================================*/

/* 测试用例: SPI独立读取 - 功能测试 */
static void _test_hal_spi_read_standalone(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };
	uint8_t rx_buffer[16] = { 0 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	/* 独立读取（发送全0或全1） */
	ret = hal_spi_read(handle, rx_buffer, sizeof(rx_buffer));
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_spi_close(handle);
}

/* 测试用例: SPI独立写入 - 功能测试 */
static void _test_hal_spi_write_standalone(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };
	uint8_t tx_buffer[16] = { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
							  0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	/* 独立写入（忽略接收数据） */
	ret = hal_spi_write(handle, tx_buffer, sizeof(tx_buffer));
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_spi_close(handle);
}

/*===========================================================================
 * 边界值测试
 *===========================================================================*/

/* 测试用例: SPI最大速度 */
static void _test_hal_spi_max_speed(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 10000000, /* 10MHz */
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_MESSAGE("SKIPPED: Max speed not supported");
		return;
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_spi_close(handle);
}

/* 测试用例: SPI最小速度 */
static void _test_hal_spi_min_speed(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 10000, /* 10KHz */
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_MESSAGE("SKIPPED: Min speed not supported");
		return;
	}

	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
	hal_spi_close(handle);
}

/* 测试用例: SPI不同位宽 */
static void _test_hal_spi_different_bits_per_word(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 16,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	int32_t ret = hal_spi_open(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_spi_close(handle);
	} else {
		TEST_MESSAGE("SKIPPED: 16-bit word size not supported");
	}
}

/* 测试用例: SPI所有模式 */
static void _test_hal_spi_all_modes(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };

	/* 测试MODE_0 */
	config.mode = SPI_MODE_0;
	int32_t ret = hal_spi_open(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_spi_close(handle);
	}

	/* 测试MODE_1 */
	config.mode = SPI_MODE_1;
	ret = hal_spi_open(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_spi_close(handle);
	}

	/* 测试MODE_2 */
	config.mode = SPI_MODE_2;
	ret = hal_spi_open(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_spi_close(handle);
	}

	/* 测试MODE_3 */
	config.mode = SPI_MODE_3;
	ret = hal_spi_open(&config, &handle);
	if (ret == OSAL_SUCCESS) {
		hal_spi_close(handle);
	}
}

/* 测试用例: SPI大数据传输 */
static void _test_hal_spi_large_transfer(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 5000 };
	uint8_t tx_buffer[4096];
	uint8_t rx_buffer[4096];

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	/* 填充测试数据 */
	for (uint32_t i = 0; i < sizeof(tx_buffer); i++) {
		tx_buffer[i] = (uint8_t)(i & 0xFF);
	}

	/* 大数据传输 */
	ret = hal_spi_transfer(handle, tx_buffer, rx_buffer, sizeof(tx_buffer));
	TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

	hal_spi_close(handle);
}

/* 测试用例: SPI零长度传输 */
static void _test_hal_spi_zero_length_transfer(void)
{
	hal_spi_handle_t handle = NULL;
	hal_spi_config_t config = { .device = "/dev/spidev0.0",
								.mode = SPI_MODE_0,
								.bits_per_word = 8,
								.max_speed_hz = 1000000,
								.timeout = 1000 };
	uint8_t buffer[4];

	int32_t ret = hal_spi_open(&config, &handle);
	if (OSAL_SUCCESS != ret) {
		TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
	}

	/* 零长度传输应该失败或返回0 */
	ret = hal_spi_transfer(handle, buffer, buffer, 0);
	TEST_ASSERT_TRUE(ret == OSAL_SUCCESS || ret < 0);

	hal_spi_close(handle);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

/* 初始化和清理 */
/* 读写操作 */
/* 传输操作 */
/* 配置操作 */

/* 测试用例数组 - 使用函数指针数组 */
static const test_case_t test_cases[] = {
	{ .name = "test_hal_spi_open_success",
	  .func = _test_hal_spi_open_success,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_open_null_config",
	  .func = _test_hal_spi_open_null_config,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_open_null_handle",
	  .func = _test_hal_spi_open_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_open_invalid_device",
	  .func = _test_hal_spi_open_invalid_device,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_close",
	  .func = _test_hal_spi_close,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_close_null_handle",
	  .func = _test_hal_spi_close_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_write_null_handle",
	  .func = _test_hal_spi_write_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_write_null_buffer",
	  .func = _test_hal_spi_write_null_buffer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_read_null_handle",
	  .func = _test_hal_spi_read_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_read_null_buffer",
	  .func = _test_hal_spi_read_null_buffer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_transfer_null_handle",
	  .func = _test_hal_spi_transfer_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_transfer_loopback",
	  .func = _test_hal_spi_transfer_loopback,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_transfer_multi_null_handle",
	  .func = _test_hal_spi_transfer_multi_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_transfer_multi_null_transfers",
	  .func = _test_hal_spi_transfer_multi_null_transfers,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_set_config_null_handle",
	  .func = _test_hal_spi_set_config_null_handle,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_set_config_null_config",
	  .func = _test_hal_spi_set_config_null_config,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_set_config_change_mode",
	  .func = _test_hal_spi_set_config_change_mode,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_read_standalone",
	  .func = _test_hal_spi_read_standalone,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_write_standalone",
	  .func = _test_hal_spi_write_standalone,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_max_speed",
	  .func = _test_hal_spi_max_speed,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_min_speed",
	  .func = _test_hal_spi_min_speed,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_different_bits_per_word",
	  .func = _test_hal_spi_different_bits_per_word,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_all_modes",
	  .func = _test_hal_spi_all_modes,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_large_transfer",
	  .func = _test_hal_spi_large_transfer,
	  .setup = NULL,
	  .teardown = NULL },
	{ .name = "test_hal_spi_zero_length_transfer",
	  .func = _test_hal_spi_zero_length_transfer,
	  .setup = NULL,
	  .teardown = NULL },
};

/* 测试套件定义 */
static const test_suite_t test_suite = {
	.suite_name = "hal_spi",
	.module_name = "hal_spi",
	.layer_name = "HAL",
	.cases = test_cases,
	.case_count = OSAL_sizeof(test_cases) / OSAL_sizeof(test_case_t),
	.suite_setup = NULL,
	.suite_teardown = NULL,
	.metadata = { .category = TEST_CATEGORY_UNIT,
				  .tags = TEST_TAG_FAST,
				  .timeout_ms = 100,
				  .description = "HAL hal_spi tests" }
};

/* 测试套件注册函数 */
void register_hal_spi_tests(void)
{
	libutest_register_suite(&test_suite);
}
