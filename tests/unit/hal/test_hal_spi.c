/**
 * @file test_hal_spi.c
 * @brief HAL SPI驱动单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "hal_spi.h"
#include "osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: SPI打开 - 成功 */
TEST_CASE(test_hal_spi_open_success)
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
        TEST_SKIP_IF(true, "/dev/spidev0.0 not available");
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    HAL_SPI_Close(handle);
}

/* 测试用例: SPI打开 - 空配置 */
TEST_CASE(test_hal_spi_open_null_config)
{
    hal_spi_handle_t handle = NULL;

    int32_t ret = HAL_SPI_Open(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI打开 - 空句柄 */
TEST_CASE(test_hal_spi_open_null_handle)
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
TEST_CASE(test_hal_spi_open_invalid_device)
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
TEST_CASE(test_hal_spi_close)
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
        TEST_SKIP_IF(true, "/dev/spidev0.0 not available");
    }

    ret = HAL_SPI_Close(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI关闭 - 空句柄 */
TEST_CASE(test_hal_spi_close_null_handle)
{
    int32_t ret = HAL_SPI_Close(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 读写操作测试
 *===========================================================================*/

/* 测试用例: SPI写入 - 空句柄 */
TEST_CASE(test_hal_spi_write_null_handle)
{
    uint8_t buffer[4] = {0x01, 0x02, 0x03, 0x04};

    int32_t ret = HAL_SPI_Write(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI写入 - 空缓冲区 */
TEST_CASE(test_hal_spi_write_null_buffer)
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
        TEST_SKIP_IF(true, "/dev/spidev0.0 not available");
    }

    ret = HAL_SPI_Write(handle, NULL, 4);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
}

/* 测试用例: SPI读取 - 空句柄 */
TEST_CASE(test_hal_spi_read_null_handle)
{
    uint8_t buffer[4];

    int32_t ret = HAL_SPI_Read(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI读取 - 空缓冲区 */
TEST_CASE(test_hal_spi_read_null_buffer)
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
        TEST_SKIP_IF(true, "/dev/spidev0.0 not available");
    }

    ret = HAL_SPI_Read(handle, NULL, 4);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
}

/*===========================================================================
 * 传输操作测试
 *===========================================================================*/

/* 测试用例: SPI全双工传输 - 空句柄 */
TEST_CASE(test_hal_spi_transfer_null_handle)
{
    uint8_t tx_buffer[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t rx_buffer[4];

    int32_t ret = HAL_SPI_Transfer(NULL, tx_buffer, rx_buffer, sizeof(tx_buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI全双工传输 - 回环测试 */
TEST_CASE(test_hal_spi_transfer_loopback)
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
        TEST_SKIP_IF(true, "/dev/spidev0.0 not available");
    }

    ret = HAL_SPI_Transfer(handle, tx_buffer, rx_buffer, sizeof(tx_buffer));

    /* 注意: 需要硬件回环才能验证数据 */
    if (OSAL_SUCCESS == ret) {
        LOG_INFO("TEST", "SPI transfer completed (loopback test requires hardware)");
    }

    HAL_SPI_Close(handle);
}

/* 测试用例: SPI批量传输 - 空句柄 */
TEST_CASE(test_hal_spi_transfer_multi_null_handle)
{
    uint8_t tx_buffer[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t rx_buffer[4];
    spi_transfer_t xfer = {
        .tx_buf = tx_buffer,
        .rx_buf = rx_buffer,
        .len = sizeof(tx_buffer),
        .speed_hz = 0,
        .delay_usecs = 0,
        .bits_per_word = 0,
        .cs_change = 0
    };

    int32_t ret = HAL_SPI_TransferMulti(NULL, &xfer, 1);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: SPI批量传输 - 空传输数组 */
TEST_CASE(test_hal_spi_transfer_multi_null_transfers)
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
        TEST_SKIP_IF(true, "/dev/spidev0.0 not available");
    }

    ret = HAL_SPI_TransferMulti(handle, NULL, 1);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
}

/*===========================================================================
 * 配置操作测试
 *===========================================================================*/

/* 测试用例: SPI设置配置 - 空句柄 */
TEST_CASE(test_hal_spi_set_config_null_handle)
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
TEST_CASE(test_hal_spi_set_config_null_config)
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
        TEST_SKIP_IF(true, "/dev/spidev0.0 not available");
    }

    ret = HAL_SPI_SetConfig(handle, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_SPI_Close(handle);
}

/* 测试用例: SPI设置配置 - 更改模式 */
TEST_CASE(test_hal_spi_set_config_change_mode)
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
        TEST_SKIP_IF(true, "/dev/spidev0.0 not available");
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

TEST_SUITE_BEGIN(test_hal_spi, "test_hal_spi", "HAL")
    /* 初始化和清理 */
    TEST_CASE_REF(test_hal_spi_open_success)
    TEST_CASE_REF(test_hal_spi_open_null_config)
    TEST_CASE_REF(test_hal_spi_open_null_handle)
    TEST_CASE_REF(test_hal_spi_open_invalid_device)
    TEST_CASE_REF(test_hal_spi_close)
    TEST_CASE_REF(test_hal_spi_close_null_handle)

    /* 读写操作 */
    TEST_CASE_REF(test_hal_spi_write_null_handle)
    TEST_CASE_REF(test_hal_spi_write_null_buffer)
    TEST_CASE_REF(test_hal_spi_read_null_handle)
    TEST_CASE_REF(test_hal_spi_read_null_buffer)

    /* 传输操作 */
    TEST_CASE_REF(test_hal_spi_transfer_null_handle)
    TEST_CASE_REF(test_hal_spi_transfer_loopback)
    TEST_CASE_REF(test_hal_spi_transfer_multi_null_handle)
    TEST_CASE_REF(test_hal_spi_transfer_multi_null_transfers)

    /* 配置操作 */
    TEST_CASE_REF(test_hal_spi_set_config_null_handle)
    TEST_CASE_REF(test_hal_spi_set_config_null_config)
    TEST_CASE_REF(test_hal_spi_set_config_change_mode)
TEST_SUITE_END(test_hal_spi, "test_hal_spi", "HAL")
