#include "test_framework.h"
/**
 * @file test_hal_can.c
 * @brief HAL CAN驱动单元测试
 */

#include "hal/hal.h"
#include "osal/osal.h"

/*===========================================================================
 * 初始化和清理测试
 *===========================================================================*/

/* 测试用例: CAN初始化 - 成功 */
TEST_CASE(test_hal_can_init_success)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);

    /* 如果初始化失败，跳过所有CAN测试 */
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // CAN interface not available or down. Please run: ip link set can0 up

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    /* 尝试发送一个测试帧，检查接口是否真正可用 */
    hal_can_frame_t test_frame = {
        .can_id = 0x001,
        .dlc = 1,
        .data = {0x00}
    };
    ret = HAL_CAN_Send(handle, &test_frame);

    /* 如果发送失败（Network is down），跳过所有CAN测试 */
    if (ret != OSAL_SUCCESS) {
        HAL_CAN_Deinit(handle);
        TEST_ASSERT_FALSE(true); // CAN interface is down. Please run: ip link set can0 type can bitrate 500000 && ip link set can0 up
    }

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
        .interface = "can0",
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
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

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
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };
    hal_can_frame_t frame = {
        .can_id = 0x123,
        .dlc = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_CAN_Send(handle, &frame);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN发送 - 空句柄 */
TEST_CASE(test_hal_can_send_null_handle)
{
    hal_can_frame_t frame = {
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
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_CAN_Send(handle, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN接收 - 超时 */
TEST_CASE(test_hal_can_recv_timeout)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 100,
        .tx_timeout = 1000
    };
    hal_can_frame_t frame;

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_CAN_Recv(handle, &frame, 100);
    TEST_ASSERT_EQUAL(OSAL_ERR_TIMEOUT, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN接收 - 空句柄 */
TEST_CASE(test_hal_can_recv_null_handle)
{
    hal_can_frame_t frame;

    int32_t ret = HAL_CAN_Recv(NULL, &frame, 100);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/* 测试用例: CAN接收 - 空帧 */
TEST_CASE(test_hal_can_recv_null_frame)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_CAN_Recv(handle, NULL, 100);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/* 测试用例: CAN发送接收回环 */
TEST_CASE(test_hal_can_loopback)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };
    hal_can_frame_t tx_frame = {
        .can_id = 0x456,
        .dlc = 4,
        .data = {0xAA, 0xBB, 0xCC, 0xDD}
    };
    hal_can_frame_t rx_frame;

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 发送帧 */
    ret = HAL_CAN_Send(handle, &tx_frame);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 接收帧（需要外部回环或另一个CAN节点） */
    ret = HAL_CAN_Recv(handle, &rx_frame, 1000);
    if (OSAL_SUCCESS == ret) {
        TEST_ASSERT_EQUAL(tx_frame.can_id, rx_frame.can_id);
        TEST_ASSERT_EQUAL(tx_frame.dlc, rx_frame.dlc);
        int32_t i;

        for (i = 0; i < tx_frame.dlc; i++) {
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
        .interface = "can0",
        .baudrate = 500000,
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

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

/*===========================================================================
 * 配置参数测试
 *===========================================================================*/

/* 测试用例: 不同波特率 */
TEST_CASE(test_hal_can_different_baudrate)
{
    hal_can_handle_t handle = NULL;
    hal_can_config_t config = {
        .interface = "can0",
        .baudrate = 250000,  /* 250kbps */
        .rx_timeout = 1000,
        .tx_timeout = 1000
    };

    int32_t ret = HAL_CAN_Init(&config, &handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    HAL_CAN_Deinit(handle);
}

/*===========================================================================
 * 测试模块注册
 *===========================================================================*/

TEST_MODULE_BEGIN(test_hal_can, "HAL")
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
//     TEST_CASE_REF(test_hal_can_get_stats_success)
//     TEST_CASE_REF(test_hal_can_get_stats_null_handle)
//     TEST_CASE_REF(test_hal_can_get_stats_null_pointer)
//     TEST_CASE_REF(test_hal_can_stats_accumulation)

    /* 配置参数 */
    TEST_CASE_REF(test_hal_can_different_baudrate)
TEST_MODULE_END(test_hal_can, "HAL")
