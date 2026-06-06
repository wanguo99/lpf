/************************************************************************
 * HAL层 - Watchdog驱动测试
 ************************************************************************/

#include "test_core.h"
#include "test_assert.h"
#include "test_registry.h"
#include "hal.h"
#include "hal_watchdog.h"
#include "osal.h"

/**
 * @brief 测试：初始化和反初始化
 */
TEST_CASE(test_hal_watchdog_init_deinit)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_Init(&config, &handle);

    /* 如果设备不存在，跳过测试 */
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_NOT_NULL(handle);

    ret = HAL_WATCHDOG_Deinit(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
}

/**
 * @brief 测试：NULL参数检查
 */
TEST_CASE(test_hal_watchdog_null_params)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    /* NULL config */
    int32_t ret = HAL_WATCHDOG_Init(NULL, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* NULL handle */
    ret = HAL_WATCHDOG_Init(&config, NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    /* NULL device */
    config.device = NULL;
    ret = HAL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

/**
 * @brief 测试：喂狗功能
 */
TEST_CASE(test_hal_watchdog_kick)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 喂狗 */
    ret = HAL_WATCHDOG_Kick(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 多次喂狗 */
    uint32_t i;
    for (i = 0; i < 5; i++)
    {
        ret = HAL_WATCHDOG_Kick(handle);
        TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
        OSAL_msleep(100);
    }

    /* 检查统计信息 */
    uint32_t kick_count = 0;
    ret = HAL_WATCHDOG_GetStats(handle, &kick_count);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(6, kick_count);

    HAL_WATCHDOG_Deinit(handle);
}

/**
 * @brief 测试：超时时间设置和获取
 */
TEST_CASE(test_hal_watchdog_timeout)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 0,  /* 使用默认值 */
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 获取当前超时时间 */
    uint32_t timeout = 0;
    ret = HAL_WATCHDOG_GetTimeout(handle, &timeout);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    LOG_INFO("TEST", "Current timeout: %u seconds", timeout);

    /* 尝试设置新的超时时间（某些硬件可能不支持） */
    ret = HAL_WATCHDOG_SetTimeout(handle, 60);
    if (ret == OSAL_ERR_NOT_SUPPORTED) {
        LOG_INFO("TEST", "Hardware does not support setting timeout dynamically");
        HAL_WATCHDOG_Deinit(handle);
        return;
    }
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 验证设置 */
    ret = HAL_WATCHDOG_GetTimeout(handle, &timeout);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);
    TEST_ASSERT_EQUAL(60, timeout);

    HAL_WATCHDOG_Deinit(handle);
}

/**
 * @brief 测试：获取剩余时间
 */
TEST_CASE(test_hal_watchdog_timeleft)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 喂狗 */
    ret = HAL_WATCHDOG_Kick(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 获取剩余时间（某些硬件可能不支持） */
    uint32_t timeleft = 0;
    ret = HAL_WATCHDOG_GetTimeleft(handle, &timeleft);
    if (ret == OSAL_SUCCESS)
    {
        LOG_INFO("TEST", "Time left: %u seconds", timeleft);
        TEST_ASSERT(timeleft > 0 && timeleft <= 30);
    }
    else if (ret == OSAL_ERR_NOT_SUPPORTED)
    {
        LOG_WARN("TEST", "GetTimeleft not supported by hardware");
    }

    HAL_WATCHDOG_Deinit(handle);
}

/**
 * @brief 测试：启用和禁用看门狗
 */
TEST_CASE(test_hal_watchdog_enable_disable)
{
    hal_watchdog_handle_t handle = NULL;
    hal_watchdog_config_t config = {
        .device = "/dev/watchdog",
        .timeout_sec = 30,
        .enable_on_init = false
    };

    int32_t ret = HAL_WATCHDOG_Init(&config, &handle);
    TEST_ASSERT_FALSE(ret != OSAL_SUCCESS); // Watchdog device not available

    /* 启用看门狗 */
    ret = HAL_WATCHDOG_Enable(handle);
    if (ret != OSAL_SUCCESS)
    {
        LOG_WARN("TEST", "Enable not supported, skipping");
        HAL_WATCHDOG_Deinit(handle);
        return;
    }

    /* 喂狗 */
    ret = HAL_WATCHDOG_Kick(handle);
    TEST_ASSERT_EQUAL(OSAL_SUCCESS, ret);

    /* 尝试禁用（某些硬件可能不支持） */
    ret = HAL_WATCHDOG_Disable(handle);
    if (ret == OSAL_ERR_NOT_SUPPORTED)
    {
        LOG_WARN("TEST", "Disable not supported by hardware");
    }

    HAL_WATCHDOG_Deinit(handle);
}

/**
 * @brief 测试：无效句柄
 */
TEST_CASE(test_hal_watchdog_invalid_handle)
{
    int32_t ret = HAL_WATCHDOG_Kick(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    ret = HAL_WATCHDOG_Deinit(NULL);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);

    uint32_t timeout = 0;
    ret = HAL_WATCHDOG_GetTimeout(NULL, &timeout);
    TEST_ASSERT_NOT_EQUAL(OSAL_SUCCESS, ret);
}

TEST_SUITE_BEGIN(test_hal_watchdog, "hal_watchdog", "HAL")
    TEST_CASE_REF(test_hal_watchdog_init_deinit)
    TEST_CASE_REF(test_hal_watchdog_null_params)
    TEST_CASE_REF(test_hal_watchdog_kick)
    TEST_CASE_REF(test_hal_watchdog_timeout)
    TEST_CASE_REF(test_hal_watchdog_timeleft)
    TEST_CASE_REF(test_hal_watchdog_enable_disable)
    TEST_CASE_REF(test_hal_watchdog_invalid_handle)
TEST_SUITE_END(test_hal_watchdog, "test_hal_watchdog", "HAL")
