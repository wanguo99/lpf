#include <test/test_framework.h>
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
static void test_hal_spi_open_success(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, &handle);

    if (OSAL_SUCCESS != ret) {
        TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    HAL_SPI_Close(handle);
}

/* 测试用例: SPI打开 - 空配置 */
static void test_hal_spi_open_null_config(void)
{
    hal_spi_handle_t handle = NULL;

    int32_t ret = HAL_SPI_Open(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI打开 - 空句柄 */
static void test_hal_spi_open_null_handle(void)
{
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI打开 - 无效设备 */
static void test_hal_spi_open_invalid_device(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev99.99",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI关闭 */
static void test_hal_spi_close(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
    }

    ret = HAL_SPI_Close(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI关闭 - 空句柄 */
static void test_hal_spi_close_null_handle(void)
{
    int32_t ret = HAL_SPI_Close(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 读写操作测试
 *===========================================================================*/

/* 测试用例: SPI写入 - 空句柄 */
static void test_hal_spi_write_null_handle(void)
{
    uint8_t buffer[4] = {0x01, 0x02, 0x03, 0x04};

    int32_t ret = HAL_SPI_Write(NULL, buffer, OSAL_sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI写入 - 空缓冲区 */
static void test_hal_spi_write_null_buffer(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
    }

    ret = HAL_SPI_Write(handle, NULL, 4);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
}

/* 测试用例: SPI读取 - 空句柄 */
static void test_hal_spi_read_null_handle(void)
{
    uint8_t buffer[4];

    int32_t ret = HAL_SPI_Read(NULL, buffer, OSAL_sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI读取 - 空缓冲区 */
static void test_hal_spi_read_null_buffer(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
    }

    ret = HAL_SPI_Read(handle, NULL, 4);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
}

/*===========================================================================
 * 传输操作测试
 *===========================================================================*/

/* 测试用例: SPI全双工传输 - 空句柄 */
static void test_hal_spi_transfer_null_handle(void)
{
    uint8_t tx_buffer[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t rx_buffer[4];

    int32_t ret = HAL_SPI_Transfer(NULL, tx_buffer, rx_buffer, OSAL_sizeof(tx_buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI全双工传输 - 回环测试 */
static void test_hal_spi_transfer_loopback(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };
    uint8_t tx_buffer[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t rx_buffer[4] = {0};

    int32_t ret = HAL_SPI_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
    }

    ret = HAL_SPI_Transfer(handle, tx_buffer, rx_buffer, OSAL_sizeof(tx_buffer));

    /* 注意: 需要硬件回环才能验证数据 */
    if (OSAL_SUCCESS == ret) {
        LOG_INFO("TEST", "SPI transfer completed (loopback test requires hardware)");
    }

    HAL_SPI_Close(handle);
}

/* 测试用例: SPI批量传输 - 空句柄 */
static void test_hal_spi_transfer_multi_null_handle(void)
{
    uint8_t tx_buffer[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t rx_buffer[4];
    hal_spi_transfer_t xfer = {
        .tx_buf = tx_buffer,
        .rx_buf = rx_buffer,
        .len = OSAL_sizeof(tx_buffer),
        .speed_hz = 0,
        .delay_usecs = 0,
        .bits_per_word = 0,
        .cs_change = 0
    };

    int32_t ret = HAL_SPI_TransferMulti(NULL, &xfer, 1);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI批量传输 - 空传输数组 */
static void test_hal_spi_transfer_multi_null_transfers(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
    }

    ret = HAL_SPI_TransferMulti(handle, NULL, 1);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
}

/*===========================================================================
 * 配置操作测试
 *===========================================================================*/

/* 测试用例: SPI设置配置 - 空句柄 */
static void test_hal_spi_set_config_null_handle(void)
{
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_1,
        .bits_per_word = 8,
        .max_speed_hz = 500000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_SetConfig(NULL, &config);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI设置配置 - 空配置 */
static void test_hal_spi_set_config_null_config(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
    }

    ret = HAL_SPI_SetConfig(handle, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
}

/* 测试用例: SPI设置配置 - 更改模式 */
static void test_hal_spi_set_config_change_mode(void)
{
    hal_spi_handle_t handle = NULL;
    hal_spi_config_t config = {
        .device = "/dev/spidev0.0",
        .mode = SPI_MODE_0,
        .bits_per_word = 8,
        .max_speed_hz = 1000000,
        .timeout = 1000
    };

    int32_t ret = HAL_SPI_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_ASSERT_FALSE(true); // /dev/spidev0.0 not available
    }

    /* 更改为MODE_1 */
    config.mode = SPI_MODE_1;
    ret = HAL_SPI_SetConfig(handle, &config);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
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
	{
		.name = "test_hal_spi_open_success",
		.func = test_hal_spi_open_success,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_open_null_config",
		.func = test_hal_spi_open_null_config,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_open_null_handle",
		.func = test_hal_spi_open_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_open_invalid_device",
		.func = test_hal_spi_open_invalid_device,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_close",
		.func = test_hal_spi_close,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_close_null_handle",
		.func = test_hal_spi_close_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_write_null_handle",
		.func = test_hal_spi_write_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_write_null_buffer",
		.func = test_hal_spi_write_null_buffer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_read_null_handle",
		.func = test_hal_spi_read_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_read_null_buffer",
		.func = test_hal_spi_read_null_buffer,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_transfer_null_handle",
		.func = test_hal_spi_transfer_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_transfer_loopback",
		.func = test_hal_spi_transfer_loopback,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_transfer_multi_null_handle",
		.func = test_hal_spi_transfer_multi_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_transfer_multi_null_transfers",
		.func = test_hal_spi_transfer_multi_null_transfers,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_set_config_null_handle",
		.func = test_hal_spi_set_config_null_handle,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_set_config_null_config",
		.func = test_hal_spi_set_config_null_config,
		.setup = NULL,
		.teardown = NULL
	},
	{
		.name = "test_hal_spi_set_config_change_mode",
		.func = test_hal_spi_set_config_change_mode,
		.setup = NULL,
		.teardown = NULL
	},
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
	.metadata = {
		.category = TEST_CATEGORY_UNIT,
		.tags = TEST_TAG_FAST,
		.timeout_ms = 100,
		.description = "HAL hal_spi tests"
	}
};

/* 测试套件注册函数 */
__attribute__((constructor))
static void register_hal_spi_tests(void)
{
	libutest_register_suite(&test_suite);
}
