/**
 * @file test_hal_i2c.c
 * @brief HAL I2C驱动单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "hal_i2c.h"
#include "osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: I2C打开 - 成功 */
TEST_CASE(test_hal_i2c_open_success)
{
    hal_i2c_handle_t handle = NULL;
    hal_i2c_config_t config = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, &handle);

    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/i2c-0 not available");
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    HAL_I2C_Close(handle);
}

/* 测试用例: I2C打开 - 空配置 */
TEST_CASE(test_hal_i2c_open_null_config)
{
    hal_i2c_handle_t handle = NULL;

    int32_t ret = HAL_I2C_Open(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C打开 - 空句柄 */
TEST_CASE(test_hal_i2c_open_null_handle)
{
    hal_i2c_config_t config = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C打开 - 无效设备 */
TEST_CASE(test_hal_i2c_open_invalid_device)
{
    hal_i2c_handle_t handle = NULL;
    hal_i2c_config_t config = {
        .device = "/dev/i2c-999",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C关闭 */
TEST_CASE(test_hal_i2c_close)
{
    hal_i2c_handle_t handle = NULL;
    hal_i2c_config_t config = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/i2c-0 not available");
    }

    ret = HAL_I2C_Close(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C关闭 - 空句柄 */
TEST_CASE(test_hal_i2c_close_null_handle)
{
    int32_t ret = HAL_I2C_Close(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 读写操作测试
 *===========================================================================*/

/* 测试用例: I2C写入 - 空句柄 */
TEST_CASE(test_hal_i2c_write_null_handle)
{
    uint8_t buffer[4] = {0x01, 0x02, 0x03, 0x04};

    int32_t ret = HAL_I2C_Write(NULL, 0x50, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C写入 - 空缓冲区 */
TEST_CASE(test_hal_i2c_write_null_buffer)
{
    hal_i2c_handle_t handle = NULL;
    hal_i2c_config_t config = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/i2c-0 not available");
    }

    ret = HAL_I2C_Write(handle, 0x50, NULL, 4);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_I2C_Close(handle);
}

/* 测试用例: I2C读取 - 空句柄 */
TEST_CASE(test_hal_i2c_read_null_handle)
{
    uint8_t buffer[4];

    int32_t ret = HAL_I2C_Read(NULL, 0x50, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C读取 - 空缓冲区 */
TEST_CASE(test_hal_i2c_read_null_buffer)
{
    hal_i2c_handle_t handle = NULL;
    hal_i2c_config_t config = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/i2c-0 not available");
    }

    ret = HAL_I2C_Read(handle, 0x50, NULL, 4);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_I2C_Close(handle);
}

/*===========================================================================
 * 寄存器操作测试
 *===========================================================================*/

/* 测试用例: I2C写寄存器 - 空句柄 */
TEST_CASE(test_hal_i2c_write_reg_null_handle)
{
    uint8_t buffer[4] = {0x01, 0x02, 0x03, 0x04};

    int32_t ret = HAL_I2C_WriteReg(NULL, 0x50, 0x00, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C写寄存器 - 空缓冲区 */
TEST_CASE(test_hal_i2c_write_reg_null_buffer)
{
    hal_i2c_handle_t handle = NULL;
    hal_i2c_config_t config = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/i2c-0 not available");
    }

    ret = HAL_I2C_WriteReg(handle, 0x50, 0x00, NULL, 4);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_I2C_Close(handle);
}

/* 测试用例: I2C读寄存器 - 空句柄 */
TEST_CASE(test_hal_i2c_read_reg_null_handle)
{
    uint8_t buffer[4];

    int32_t ret = HAL_I2C_ReadReg(NULL, 0x50, 0x00, buffer, sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C读寄存器 - 空缓冲区 */
TEST_CASE(test_hal_i2c_read_reg_null_buffer)
{
    hal_i2c_handle_t handle = NULL;
    hal_i2c_config_t config = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/i2c-0 not available");
    }

    ret = HAL_I2C_ReadReg(handle, 0x50, 0x00, NULL, 4);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_I2C_Close(handle);
}

/*===========================================================================
 * 传输操作测试
 *===========================================================================*/

/* 测试用例: I2C传输 - 空句柄 */
TEST_CASE(test_hal_i2c_transfer_null_handle)
{
    uint8_t buffer[4] = {0x01, 0x02, 0x03, 0x04};
    i2c_msg_t msg = {
        .addr = 0x50,
        .flags = 0,
        .len = sizeof(buffer),
        .buf = buffer
    };

    int32_t ret = HAL_I2C_Transfer(NULL, &msg, 1);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: I2C传输 - 空消息 */
TEST_CASE(test_hal_i2c_transfer_null_msgs)
{
    hal_i2c_handle_t handle = NULL;
    hal_i2c_config_t config = {
        .device = "/dev/i2c-0",
        .timeout = 1000
    };

    int32_t ret = HAL_I2C_Open(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/i2c-0 not available");
    }

    ret = HAL_I2C_Transfer(handle, NULL, 1);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_I2C_Close(handle);
}

/*===========================================================================
 * 测试套件注册
 *===========================================================================*/

TEST_SUITE_BEGIN(test_hal_i2c, "test_hal_i2c", "HAL")
    /* 初始化和清理 */
    TEST_CASE_REF(test_hal_i2c_open_success)
    TEST_CASE_REF(test_hal_i2c_open_null_config)
    TEST_CASE_REF(test_hal_i2c_open_null_handle)
    TEST_CASE_REF(test_hal_i2c_open_invalid_device)
    TEST_CASE_REF(test_hal_i2c_close)
    TEST_CASE_REF(test_hal_i2c_close_null_handle)

    /* 读写操作 */
    TEST_CASE_REF(test_hal_i2c_write_null_handle)
    TEST_CASE_REF(test_hal_i2c_write_null_buffer)
    TEST_CASE_REF(test_hal_i2c_read_null_handle)
    TEST_CASE_REF(test_hal_i2c_read_null_buffer)

    /* 寄存器操作 */
    TEST_CASE_REF(test_hal_i2c_write_reg_null_handle)
    TEST_CASE_REF(test_hal_i2c_write_reg_null_buffer)
    TEST_CASE_REF(test_hal_i2c_read_reg_null_handle)
    TEST_CASE_REF(test_hal_i2c_read_reg_null_buffer)

    /* 传输操作 */
    TEST_CASE_REF(test_hal_i2c_transfer_null_handle)
    TEST_CASE_REF(test_hal_i2c_transfer_null_msgs)
TEST_SUITE_END(test_hal_i2c, "test_hal_i2c", "HAL")
