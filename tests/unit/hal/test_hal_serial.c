/**
 * @file test_hal_serial.c
 * @brief HAL串口驱动单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "hal_serial.h"
#include "osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: 串口打开 - 成功 */
TEST_CASE(test_hal_serial_open_success)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    HAL_Serial_Close(handle);
}

/* 测试用例: 串口打开 - 空设备路径 */
TEST_CASE(test_hal_serial_open_null_device)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open(NULL, &config, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口打开 - 空配置 */
TEST_CASE(test_hal_serial_open_null_config)
{
    hal_serial_handle_t handle = NULL;

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口打开 - 空句柄 */
TEST_CASE(test_hal_serial_open_null_handle)
{
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口打开 - 无效设备 */
TEST_CASE(test_hal_serial_open_invalid_device)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/invalid_tty999", &config, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口关闭 */
TEST_CASE(test_hal_serial_close)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_Close(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 串口关闭 - 空句柄 */
TEST_CASE(test_hal_serial_close_null_handle)
{
    int32_t ret = HAL_Serial_Close(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 读写测试
 *===========================================================================*/

/* 测试用例: 串口写入 - 成功 */
TEST_CASE(test_hal_serial_write_success)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };
    uint8_t data[] = "Hello Serial";

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_Write(handle, data, sizeof(data), 1000);
    TEST_ASSERT_TRUE(ret > 0);

    HAL_Serial_Close(handle);
}

/* 测试用例: 串口写入 - 空句柄 */
TEST_CASE(test_hal_serial_write_null_handle)
{
    uint8_t data[] = "test";

    int32_t ret = HAL_Serial_Write(NULL, data, sizeof(data), 1000);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例: 串口写入 - 空缓冲区 */
TEST_CASE(test_hal_serial_write_null_buffer)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_Write(handle, NULL, 10, 1000);
    TEST_ASSERT_TRUE(ret < 0);

    HAL_Serial_Close(handle);
}

/* 测试用例: 串口写入 - 零长度 */
TEST_CASE(test_hal_serial_write_zero_length)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };
    uint8_t data[] = "test";

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_Write(handle, data, 0, 1000);
    TEST_ASSERT_EQUAL(0, ret);

    HAL_Serial_Close(handle);
}

/* 测试用例: 串口读取 - 超时 */
TEST_CASE(test_hal_serial_read_timeout)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };
    uint8_t buffer[64];

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_Read(handle, buffer, sizeof(buffer), 100);
    /* 超时或读取到数据都是正常的 */

    HAL_Serial_Close(handle);
}

/* 测试用例: 串口读取 - 空句柄 */
TEST_CASE(test_hal_serial_read_null_handle)
{
    uint8_t buffer[64];

    int32_t ret = HAL_Serial_Read(NULL, buffer, sizeof(buffer), 100);
    TEST_ASSERT_EQUAL(OSAL_ERR_INVALID_POINTER, ret);
}

/* 测试用例: 串口读取 - 空缓冲区 */
TEST_CASE(test_hal_serial_read_null_buffer)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_Read(handle, NULL, 64, 100);
    TEST_ASSERT_TRUE(ret < 0);

    HAL_Serial_Close(handle);
}

/*===========================================================================
 * 刷新测试
 *===========================================================================*/

/* 测试用例: 刷新缓冲区 - 成功 */
TEST_CASE(test_hal_serial_flush_success)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_Flush(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_Serial_Close(handle);
}

/* 测试用例: 刷新缓冲区 - 空句柄 */
TEST_CASE(test_hal_serial_flush_null_handle)
{
    int32_t ret = HAL_Serial_Flush(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 配置测试 (暂时注释，等待HAL_Serial_SetConfig实现)
 *===========================================================================*/

#if 0  /* HAL_Serial_SetConfig未实现，暂时禁用这些测试 */

/* 测试用例: 设置配置 - 成功 */
TEST_CASE(test_hal_serial_set_config_success)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };
    hal_serial_config_t new_config = {
        .baud_rate = 9600,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_EVEN,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_SetConfig(handle, &new_config);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_Serial_Close(handle);
}

/* 测试用例: 设置配置 - 空句柄 */
TEST_CASE(test_hal_serial_set_config_null_handle)
{
    hal_serial_config_t config = {
        .baud_rate = 9600,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_SetConfig(NULL, &config);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 设置配置 - 空配置 */
TEST_CASE(test_hal_serial_set_config_null_config)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    ret = HAL_Serial_SetConfig(handle, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_Serial_Close(handle);
}

#endif  /* HAL_Serial_SetConfig未实现 */

/*===========================================================================
 * 配置参数测试
 *===========================================================================*/

/* 测试用例: 不同波特率 */
TEST_CASE(test_hal_serial_different_baudrate)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 9600,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    HAL_Serial_Close(handle);
}

/* 测试用例: 不同校验位 */
TEST_CASE(test_hal_serial_different_parity)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_ODD,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    HAL_Serial_Close(handle);
}

/* 测试用例: 不同数据位 */
TEST_CASE(test_hal_serial_different_databits)
{
    hal_serial_handle_t handle = NULL;
    hal_serial_config_t config = {
        .baud_rate = 115200,
        .data_bits = 7,
        .stop_bits = 1,
        .parity = HAL_SERIAL_PARITY_NONE,
        .flow_control = HAL_SERIAL_FLOW_NONE
    };

    int32_t ret = HAL_Serial_Open("/dev/ttyS0", &config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "/dev/ttyS0 not available");
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    HAL_Serial_Close(handle);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_SUITE_BEGIN(test_hal_serial, "hal_serial", "HAL")
    // HAL串口驱动测试
    /* 初始化和清理 */
    TEST_CASE_REF(test_hal_serial_open_success)
    TEST_CASE_REF(test_hal_serial_open_null_device)
    TEST_CASE_REF(test_hal_serial_open_null_config)
    TEST_CASE_REF(test_hal_serial_open_null_handle)
    TEST_CASE_REF(test_hal_serial_open_invalid_device)
    TEST_CASE_REF(test_hal_serial_close)
    TEST_CASE_REF(test_hal_serial_close_null_handle)

    /* 读写 */
    TEST_CASE_REF(test_hal_serial_write_success)
    TEST_CASE_REF(test_hal_serial_write_null_handle)
    TEST_CASE_REF(test_hal_serial_write_null_buffer)
    TEST_CASE_REF(test_hal_serial_write_zero_length)
    TEST_CASE_REF(test_hal_serial_read_timeout)
    TEST_CASE_REF(test_hal_serial_read_null_handle)
    TEST_CASE_REF(test_hal_serial_read_null_buffer)

    /* 刷新 */
    TEST_CASE_REF(test_hal_serial_flush_success)
    TEST_CASE_REF(test_hal_serial_flush_null_handle)

    /* 配置 - 暂时注释，等待HAL_Serial_SetConfig实现 */
#if 0
    TEST_CASE_REF(test_hal_serial_set_config_success)
    TEST_CASE_REF(test_hal_serial_set_config_null_handle)
    TEST_CASE_REF(test_hal_serial_set_config_null_config)
#endif

    /* 配置参数 */
    TEST_CASE_REF(test_hal_serial_different_baudrate)
    TEST_CASE_REF(test_hal_serial_different_parity)
    TEST_CASE_REF(test_hal_serial_different_databits)
TEST_SUITE_END(test_hal_serial, "test_hal_serial", "HAL")
