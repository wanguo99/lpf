/**
 * @file test_hal_can.c
 * @brief HAL CAN驱动单元测试
 */

#include "tests_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "hal_can.h"
#include "osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: CAN初始化 - 成功 */
TEST_CASE(test_hal_can_init_success)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);

    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN初始化 - 空配置 */
TEST_CASE(test_hal_can_init_null_config)
{
    hal_can_handle_t handle = NULL;

    int32_t ret = HAL_CAN_Init(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN初始化 - 空句柄 */
TEST_CASE(test_hal_can_init_null_handle)
{
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN初始化 - 无效接口 */
TEST_CASE(test_hal_can_init_invalid_interface)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "invalid_can999",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN清理 */
TEST_CASE(test_hal_can_deinit)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    ret = HAL_CAN_Deinit(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN清理 - 空句柄 */
TEST_CASE(test_hal_can_deinit_null_handle)
{
    int32_t ret = HAL_CAN_Deinit(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 发送和接收测试
 *===========================================================================*/

/* 测试用例: CAN发送 - 成功 */
TEST_CASE(test_hal_can_send_success)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };
    can_frame_t frame = {
        .can_id = 0x123,
        .dlc = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    ret = HAL_CAN_Send(handle, &frame);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN发送 - 空句柄 */
TEST_CASE(test_hal_can_send_null_handle)
{
    can_frame_t frame = {
        .can_id = 0x123,
        .dlc = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };

    int32_t ret = HAL_CAN_Send(NULL, &frame);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN发送 - 空帧 */
TEST_CASE(test_hal_can_send_null_frame)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    ret = HAL_CAN_Send(handle, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN接收 - 超时 */
TEST_CASE(test_hal_can_recv_timeout)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 100,
        .tx_timeout = 1000
    };
    can_frame_t frame;

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    ret = HAL_CAN_Recv(handle, &frame, 100);
    TEST_ASSERT_EQUAL(OSAL_ERR_TIMEOUT, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN接收 - 空句柄 */
TEST_CASE(test_hal_can_recv_null_handle)
{
    can_frame_t frame;

    int32_t ret = HAL_CAN_Recv(NULL, &frame, 100);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN接收 - 空帧 */
TEST_CASE(test_hal_can_recv_null_frame)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    ret = HAL_CAN_Recv(handle, NULL, 100);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN发送接收回环 */
TEST_CASE(test_hal_can_loopback)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };
    can_frame_t tx_frame = {
        .can_id = 0x456,
        .dlc = 4,
        .data = {0xAA, 0xBB, 0xCC, 0xDD}
    };
    can_frame_t rx_frame;

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    /* 发送帧 */
    ret = HAL_CAN_Send(handle, &tx_frame);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 接收帧（vcan0支持回环） */
    ret = HAL_CAN_Recv(handle, &rx_frame, 1000);
    if (OSAL_SUCCESS == ret) {
        TEST_ASSERT_EQUAL(tx_frame.can_id, rx_frame.can_id);
        TEST_ASSERT_EQUAL(tx_frame.dlc, rx_frame.dlc);
        for (int i = 0; i < tx_frame.dlc; i++) {
            TEST_ASSERT_EQUAL(tx_frame.data[i], rx_frame.data[i]);
        }
    }

    HAL_CAN_Deinit(handle);
}

/*===========================================================================
 * 过滤器测试
 *===========================================================================*/

/* 测试用例: 设置过滤器 - 成功 */
TEST_CASE(test_hal_can_set_filter_success)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    ret = HAL_CAN_SetFilter(handle, 0x100, 0x7FF);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: 设置过滤器 - 空句柄 */
TEST_CASE(test_hal_can_set_filter_null_handle)
{
    int32_t ret = HAL_CAN_SetFilter(NULL, 0x100, 0x7FF);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/*===========================================================================
 * 统计信息测试
 *===========================================================================*/

/* 测试用例: 获取统计信息 - 成功 */
TEST_CASE(test_hal_can_get_stats_success)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };
    uint32_t tx_count, rx_count, err_count;

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    ret = HAL_CAN_GetStats(handle, &tx_count, &rx_count, &err_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: 获取统计信息 - 空句柄 */
TEST_CASE(test_hal_can_get_stats_null_handle)
{
    uint32_t tx_count, rx_count, err_count;

    int32_t ret = HAL_CAN_GetStats(NULL, &tx_count, &rx_count, &err_count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: 获取统计信息 - 空指针 */
TEST_CASE(test_hal_can_get_stats_null_pointer)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };
    uint32_t count;

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    ret = HAL_CAN_GetStats(handle, NULL, &count, &count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_CAN_GetStats(handle, &count, NULL, &count);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_CAN_GetStats(handle, &count, &count, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: 统计信息累加验证 */
TEST_CASE(test_hal_can_stats_accumulation)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };
    uint32_t tx1, rx1, err1, tx2, rx2, err2;
    can_frame_t frame = {
        .can_id = 0x789,
        .dlc = 2,
        .data = {0x11, 0x22}
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    /* 获取初始统计 */
    ret = HAL_CAN_GetStats(handle, &tx1, &rx1, &err1);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 发送一些帧 */
    for (int i = 0; i < 5; i++) {
        HAL_CAN_Send(handle, &frame);
    }

    /* 获取新统计 */
    ret = HAL_CAN_GetStats(handle, &tx2, &rx2, &err2);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证统计增加 */
    TEST_ASSERT_TRUE(tx2 >= tx1);

    HAL_CAN_Deinit(handle);
}

/*===========================================================================
 * 配置参数测试
 *===========================================================================*/

/* 测试用例: 不同波特率 */
TEST_CASE(test_hal_can_different_baudrate)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "vcan0",
        .baudrate = 250000,  /* 250kbps */
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    if (OSAL_SUCCESS != ret) {
        TEST_SKIP_IF(true, "vcan0 not available");
    }

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    HAL_CAN_Deinit(handle);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_SUITE_BEGIN(test_hal_can, "hal_can", "HAL")
    // HAL CAN驱动测试
    /* 初始化和清理 */
    TEST_CASE_REF(test_hal_can_init_success)
    TEST_CASE_REF(test_hal_can_init_null_config)
    TEST_CASE_REF(test_hal_can_init_null_handle)
    TEST_CASE_REF(test_hal_can_init_invalid_interface)
    TEST_CASE_REF(test_hal_can_deinit)
    TEST_CASE_REF(test_hal_can_deinit_null_handle)

    /* 发送和接收 */
    TEST_CASE_REF(test_hal_can_send_success)
    TEST_CASE_REF(test_hal_can_send_null_handle)
    TEST_CASE_REF(test_hal_can_send_null_frame)
    TEST_CASE_REF(test_hal_can_recv_timeout)
    TEST_CASE_REF(test_hal_can_recv_null_handle)
    TEST_CASE_REF(test_hal_can_recv_null_frame)
    TEST_CASE_REF(test_hal_can_loopback)

    /* 过滤器 */
    TEST_CASE_REF(test_hal_can_set_filter_success)
    TEST_CASE_REF(test_hal_can_set_filter_null_handle)

    /* 统计信息 */
    TEST_CASE_REF(test_hal_can_get_stats_success)
    TEST_CASE_REF(test_hal_can_get_stats_null_handle)
    TEST_CASE_REF(test_hal_can_get_stats_null_pointer)
    TEST_CASE_REF(test_hal_can_stats_accumulation)

    /* 配置参数 */
    TEST_CASE_REF(test_hal_can_different_baudrate)
TEST_SUITE_END(test_hal_can, "test_hal_can", "HAL")
